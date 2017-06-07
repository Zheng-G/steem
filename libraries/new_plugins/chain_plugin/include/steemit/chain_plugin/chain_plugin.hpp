#pragma once
#include <appbase/application.hpp>
#include <steemit/chain/database.hpp>

namespace steemit { namespace chain_plugin {
   using steemit::chain::database;
   using std::unique_ptr;
   using namespace appbase;

namespace chain_apis {
struct empty{};

class read_only {
   const database& db;

public:
   read_only(const database& db)
      : db(db) {}

   using get_info_params = empty;
   struct get_info_results {
      uint32_t head_block_num;
      chain::block_id_type head_block_id;
      fc::time_point_sec head_block_time;
      chain::account_name_type head_block_producer;
      fc::uint128 recent_slots;
      double participation_rate;
   };
   get_info_results get_info(const get_info_params&) const;

   struct get_block_params {
      string block_num_or_id;
   };
   using get_block_results = optional<chain::signed_block>;
   get_block_results get_block(const get_block_params& params) const;
};

class read_write {
   database& db;
public:
   read_write(database& db) : db(db) {}

   using push_block_params = chain::signed_block;
   using push_block_results = empty;
   push_block_results push_block(const push_block_params& params);

   using push_transaction_params = chain::signed_transaction;
   using push_transaction_results = empty;
   push_transaction_results push_transaction(const push_transaction_params& params);
};
} // namespace chain_apis

class chain_plugin : public plugin<chain_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES()

   chain_plugin();
   virtual ~chain_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   chain_apis::read_only get_read_only_api() const { return chain_apis::read_only(db()); }
   chain_apis::read_write get_read_write_api() { return chain_apis::read_write(db()); }

   bool accept_block(const chain::signed_block& block, bool currently_syncing);
   void accept_transaction(const chain::signed_transaction& trx);

   bool block_is_on_preferred_chain(const chain::block_id_type& block_id);

   database& db();
   const database& db() const;

private:
   unique_ptr<class chain_plugin_impl> my;
};

} } // steemit::chain_plugin

FC_REFLECT(steemit::chain_plugin::chain_apis::empty, )
FC_REFLECT(steemit::chain_plugin::chain_apis::read_only::get_info_results,
           (head_block_num)(head_block_id)(head_block_time)(head_block_producer)
           (recent_slots)(participation_rate))
FC_REFLECT(steemit::chain_plugin::chain_apis::read_only::get_block_params, (block_num_or_id))
