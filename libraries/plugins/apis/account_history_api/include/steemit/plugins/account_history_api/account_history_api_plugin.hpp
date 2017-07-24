#pragma once
#include <steemit/plugins/account_history/account_history_plugin.hpp>
#include <steemit/plugins/json_rpc/json_rpc_plugin.hpp>

#include <appbase/application.hpp>

#define ACCOUNT_HISTORY_API_PLUGIN_NAME "account_history_api"


namespace steemit { namespace plugins { namespace account_history_api {

using namespace appbase;

class account_history_api_plugin : public plugin< account_history_api_plugin >
{
public:
   APPBASE_PLUGIN_REQUIRES(
      (steemit::plugins::account_history::account_history_plugin)
      (steemit::plugins::json_rpc::json_rpc_plugin)
   )

   account_history_api_plugin();
   virtual ~account_history_api_plugin();

   virtual void set_program_options( options_description& cli, options_description& cfg ) override;

   void plugin_initialize( const variables_map& options );
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr< class account_history_api > _api;
};

} } } // steemit::plugins::account_history_api
