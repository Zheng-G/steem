#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <boost/algorithm/string.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#define JSON_RPC_PARSE_ERROR        (-32700)
#define JSON_RPC_INVALID_REQUEST    (-32600)
#define JSON_RPC_METHOD_NOT_FOUND   (-32601)
#define JSON_RPC_INVALID_PARAMS     (-32602)
#define JSON_RPC_INTERNAL_ERROR     (-32603)
#define JSON_RPC_SERVER_ERROR       (-32000)

namespace steemit { namespace plugins { namespace json_rpc {

namespace detail
{
   struct json_rpc_error
   {
      json_rpc_error( int32_t c, std::string m, fc::optional< fc::variant > d = fc::optional< fc::variant >() )
         : code( c ), message( m ), data( d ) {}

      int32_t                          code;
      std::string                      message;
      fc::optional< fc::variant >      data;
   };

   struct json_rpc_response
   {
      std::string                      jsonrpc = "2.0";
      fc::optional< fc::variant >      result;
      fc::optional< json_rpc_error >   error;
      fc::variant                      id;
   };

   class json_rpc_plugin_impl
   {
      public:
         json_rpc_plugin_impl();
         ~json_rpc_plugin_impl();

         void add_api_method( const string& api_name, const string& method_name, const api_method& api );
         json_rpc_response rpc( const fc::variant& message );

         map< string, api_description > _registered_apis;
   };

   json_rpc_plugin_impl::json_rpc_plugin_impl() {}
   json_rpc_plugin_impl::~json_rpc_plugin_impl() {}

   void json_rpc_plugin_impl::add_api_method( const string& api_name, const string& method_name, const api_method& api )
   {
      _registered_apis[ api_name ][ method_name ] = api;
   }

   json_rpc_response json_rpc_plugin_impl::rpc( const fc::variant& message )
   {
      json_rpc_response response;

      try
      {
         const auto request = message.get_object();

         if( request.contains( "id" ) )
         {
            try
            {
               response.id = request[ "id" ].as_int64();
            }
            catch( fc::exception& )
            {
               try
               {
                  response.id = request[ "id" ].as_string();
               }
               catch( fc::exception& ) {}
            }
         }

         if( request.contains( "jsonrpc" ) && request[ "jsonrpc" ].as_string() == "2.0" )
         {
            if( request.contains( "method" ) )
            {
               try
               {
                  string method = request[ "method" ].as_string();

                  api_method* call;
                  fc::variant params;

                  // This is to maintain backwards compatibility with existing call structure.
                  if( method == "call" )
                  {
                     std::vector< fc::variant > v = request[ "params" ].as< std::vector< fc::variant > >();
                     FC_ASSERT( v.size() == 2 || v.size() == 3, "params should be {\"api\", \"method\", \"args\"" );

                     auto api_itr = _registered_apis.find( v[0].as_string() );
                     FC_ASSERT( api_itr != _registered_apis.end(), "Could not find API ${api}", ("api", v[0]) );

                     auto method_itr = api_itr->second.find( v[1].as_string() );
                     FC_ASSERT( method_itr != api_itr->second.end(), "Could not find method ${method}", ("method", v[1]) );

                     call = &(method_itr->second);

                     if( v.size() == 3 )
                     {
                        if( fc::json::to_string( v[2] ) == "[]" ) // void_type generated by fc API library
                           params = fc::json::from_string( "{}" );
                        else
                           params = v[2];
                     }
                     else
                        params = fc::json::from_string( "{}" );
                  }
                  else
                  {
                     vector< std::string > v;
                     boost::split( v, method, boost::is_any_of( "." ) );

                     FC_ASSERT( v.size() == 2, "method specification invalid. Should be api.method" );

                     auto api_itr = _registered_apis.find( v[0] );
                     FC_ASSERT( api_itr != _registered_apis.end(), "Could not find API ${api}", ("api", v[0]) );

                     auto method_itr = api_itr->second.find( v[1] );
                     FC_ASSERT( method_itr != api_itr->second.end(), "Could not find method ${method}", ("method", v[1]) );

                     call = &(method_itr->second);
                     params = request.contains( "params" ) ? request[ "params" ] : fc::json::from_string( "{}" );
                  }
                  response.result = (*call)( params );
               }
               catch( fc::assert_exception& e )
               {
                  response.error = json_rpc_error( JSON_RPC_METHOD_NOT_FOUND, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
               }
            }
         }
         else
         {
            response.error = json_rpc_error( JSON_RPC_INVALID_REQUEST, "jsonrpc value is not \"2.0\"" );
         }
      }
      catch( fc::parse_error_exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_INVALID_PARAMS, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( fc::bad_cast_exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_INVALID_PARAMS, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( fc::exception& e )
      {
         response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      }
      catch( ... )
      {
         response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, "Unknown error - parsing rpc message failed" );
      }

      return response;
   }
}

using detail::json_rpc_error;
using detail::json_rpc_response;

json_rpc_plugin::json_rpc_plugin() : _my( new detail::json_rpc_plugin_impl() ) {}
json_rpc_plugin::~json_rpc_plugin() {}

void json_rpc_plugin::plugin_initialize( const variables_map& options ) {}
void json_rpc_plugin::plugin_startup() {}
void json_rpc_plugin::plugin_shutdown() {}

void json_rpc_plugin::add_api_method( const string& api_name, const string& method_name, const api_method& api )
{
   _my->add_api_method( api_name, method_name, api );
}

string json_rpc_plugin::call( const string& message )
{
   wdump( (message) );

   try
   {
      fc::variant v = fc::json::from_string( message );

      if( v.is_array() )
      {
         vector< fc::variant > messages = v.as< vector< fc::variant > >();
         vector< json_rpc_response > responses;
         responses.reserve( messages.size() );

         for( auto& m : messages )
            responses.push_back( _my->rpc( m ) );

         return fc::json::to_string( responses );
      }
      else
      {
         return fc::json::to_string( _my->rpc( v ) );
      }
   }
   catch( fc::exception& e )
   {
      json_rpc_response response;
      response.error = json_rpc_error( JSON_RPC_SERVER_ERROR, e.to_string(), fc::variant( *(e.dynamic_copy_exception()) ) );
      return fc::json::to_string( response );
   }

}

} } } // steemit::plugins::json_rpc

FC_REFLECT( steemit::plugins::json_rpc::detail::json_rpc_error, (code)(message)(data) )
FC_REFLECT( steemit::plugins::json_rpc::detail::json_rpc_response, (jsonrpc)(result)(error)(id) )
