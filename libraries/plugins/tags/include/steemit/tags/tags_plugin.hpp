#pragma once

#include <steemit/app/plugin.hpp>
#include <steemit/chain/database.hpp>
#include <steemit/chain/comment_object.hpp>

#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <fc/thread/future.hpp>
#include <fc/api.hpp>

namespace steemit { namespace tags {
using namespace steemit::chain;
using namespace boost::multi_index;

using steemit::app::application;
//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef TAG_SPACE_ID
#define TAG_SPACE_ID 5
#endif


enum tags_object_type
{
   key_account_object_type = 0,
   bucket_object_type = 1,///< used in market_history_plugin
   message_object_type = 2,
   tag_object_type = 3,
   tag_stats_object_type = 4,
   peer_stats_object_type = 5
};


namespace detail { class tags_plugin_impl; }


/**
 *  The purpose of the tag object is to allow the generation and listing of
 *  all top level posts by a string tag.  The desired sort orders include:
 *
 *  1. created - time of creation
 *  2. maturing - about to receive a payout
 *  3. active - last reply the post or any child of the post
 *  4. netvotes - individual accounts voting for post minus accounts voting against it
 *
 *  When ever a comment is modified, all tag_objects for that comment are updated to match.
 */
class  tag_object : public abstract_object<tag_object> {
   public:
      static const uint8_t space_id = TAG_SPACE_ID;
      static const uint8_t type_id  = tag_object_type;

      string             tag;
      time_point_sec     created;
      time_point_sec     active;
      time_point_sec     cashout;
      int64_t            net_rshares = 0;
      int32_t            net_votes   = 0;
      int32_t            children    = 0;
      double             hot         = 0;
      share_type         promoted_balance = 0;

      /**
       *  Used to track the total rshares^2 of all children, this is used for indexing purposes. A discussion
       *  that has a nested comment of high value should promote the entire discussion so that the comment can
       *  be reviewed.
       */
      fc::uint128_t     children_rshares2;
      asset             total_payout = asset( 0, SBD_SYMBOL );
      comment_mode      mode;

      account_id_type   author;
      comment_id_type   parent;
      comment_id_type   comment;
};


struct by_cashout; /// all posts regardless of depth
struct by_net_rshares; /// all comments regardless of depth
struct by_parent_created;
struct by_parent_active;
struct by_parent_promoted;
struct by_parent_net_rshares; /// all top level posts by direct pending payout
struct by_parent_net_votes; /// all top level posts by direct votes
struct by_parent_children_rshares2; /// all top level posts by total cumulative payout (aka trending)
struct by_parent_children; /// all top level posts with the most discussion (replies at all levels)
struct by_parent_hot;
struct by_author_parent_created;  /// all blog posts by author with tag
struct by_author_comment;
struct by_mode_parent_children_rshares2;
struct by_comment;
struct by_tag;


typedef multi_index_container<
   tag_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_non_unique< tag< by_comment >, member< tag_object, comment_id_type, &tag_object::comment > >,
      ordered_unique< tag< by_author_comment >,
            composite_key< tag_object,
               member< tag_object, account_id_type, &tag_object::author >,
               member< tag_object, comment_id_type, &tag_object::comment >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<account_id_type>, std::less<comment_id_type>, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_parent_created >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, time_point_sec, &tag_object::created >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<comment_id_type>, std::greater< time_point_sec >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_parent_active >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, time_point_sec, &tag_object::active >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<comment_id_type>, std::greater< time_point_sec >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_parent_promoted >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, share_type, &tag_object::promoted_balance >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<comment_id_type>, std::greater< share_type >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_parent_net_rshares >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, int64_t, &tag_object::net_rshares >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<comment_id_type>, std::greater< int64_t >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_parent_net_votes >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, int32_t, &tag_object::net_votes >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<comment_id_type>, std::greater< int32_t >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_parent_children >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, int32_t, &tag_object::children >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<comment_id_type>, std::greater< int32_t >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_parent_hot >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, double, &tag_object::hot >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<comment_id_type>, std::greater< double >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_parent_children_rshares2 >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, fc::uint128_t, &tag_object::children_rshares2 >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<comment_id_type>, std::greater< fc::uint128_t >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_mode_parent_children_rshares2 >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, comment_mode, &tag_object::mode >,
               member< tag_object, comment_id_type, &tag_object::parent >,
               member< tag_object, fc::uint128_t, &tag_object::children_rshares2 >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less< comment_mode >, std::less<comment_id_type>, std::greater< fc::uint128_t >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_cashout >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, time_point_sec, &tag_object::cashout >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less< time_point_sec >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_net_rshares >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, int64_t, &tag_object::net_rshares >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::greater< int64_t >, std::less< object_id_type > >
      >,
      ordered_unique< tag< by_author_parent_created >,
            composite_key< tag_object,
               member< tag_object, string, &tag_object::tag >,
               member< tag_object, account_id_type, &tag_object::author >,
               member< tag_object, time_point_sec, &tag_object::created >,
               member<object, object_id_type, &object::id >
            >,
            composite_key_compare< std::less<string>, std::less<account_id_type>, std::greater< time_point_sec >, std::less< object_id_type > >
      >
   >
> tag_multi_index_type;

typedef graphene::db::generic_index< tag_object, tag_multi_index_type> tag_index;


/**
 *  The purpose of this index is to quickly identify how popular various tags by maintaining variou sums over
 *  all posts under a particular tag
 */
class tag_stats_object : public abstract_object<tag_stats_object> {
   public:
      static const uint8_t space_id = TAG_SPACE_ID;
      static const uint8_t type_id  = tag_stats_object_type;

      string            tag;
      fc::uint128_t     total_children_rshares2;
      asset             total_payout = asset( 0, SBD_SYMBOL );
      int32_t           net_votes = 0;
      uint32_t          top_posts = 0;
      uint32_t          comments  = 0;
};

struct by_comments;
struct by_top_posts;
struct by_trending;

typedef multi_index_container<
   tag_stats_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_tag >, member< tag_stats_object, string, &tag_stats_object::tag > >,
      ordered_non_unique< tag< by_comments >,
         composite_key< tag_stats_object,
            member< tag_stats_object, string, &tag_stats_object::tag >,
            member< tag_stats_object, uint32_t, &tag_stats_object::comments >
         >,
         composite_key_compare< std::less<string>, std::greater<uint32_t> >
      >,
      ordered_non_unique< tag< by_top_posts >,
         composite_key< tag_stats_object,
            member< tag_stats_object, string, &tag_stats_object::tag >,
            member< tag_stats_object, uint32_t, &tag_stats_object::top_posts >
         >,
         composite_key_compare< std::less<string>, std::greater<uint32_t> >
      >,
      ordered_non_unique< tag< by_trending >,
         composite_key< tag_stats_object,
            member< tag_stats_object, string, &tag_stats_object::tag >,
            member< tag_stats_object, fc::uint128_t, &tag_stats_object::total_children_rshares2 >
         >,
         composite_key_compare< std::less<string>, std::greater<uint128_t> >
      >
  >
> tag_stats_multi_index_type;

typedef graphene::db::generic_index< tag_stats_object, tag_stats_multi_index_type> tag_stats_index;


/**
 *  The purpose of this object is to track the relationship between accounts based upon how a user votes. Every time
 *  a user votes on a post, the relationship between voter and author increases direct rshares.
 */
class  peer_stats_object : public abstract_object<peer_stats_object> {
  public:
      static const uint8_t space_id = TAG_SPACE_ID;
      static const uint8_t type_id  = peer_stats_object_type;

      account_id_type voter;
      account_id_type peer;
      int32_t         direct_positive_votes = 0;
      int32_t         direct_votes = 1;

      int32_t         indirect_positive_votes = 0;
      int32_t         indirect_votes = 1;

      float           rank = 0;

      void update_rank() {
          auto direct         = float( direct_positive_votes ) / direct_votes;
          auto indirect       = float( indirect_positive_votes ) / indirect_votes;
          auto direct_order   = log( direct_votes );
          auto indirect_order = log( indirect_votes );

          if( !(direct_positive_votes+indirect_positive_votes) ){
            direct_order *= -1;
            indirect_order *= -1;
          }

          direct *= direct;
          indirect *= indirect;

          direct *= direct_order * 10;
          indirect *= indirect_order;

          rank = direct + indirect;
      }
};


struct by_rank;
struct by_voter_peer;
typedef multi_index_container<
   peer_stats_object,
   indexed_by<
      ordered_unique< tag< by_id >, member< object, object_id_type, &object::id > >,
      ordered_unique< tag< by_rank >,
         composite_key< peer_stats_object,
            member< peer_stats_object, account_id_type, &peer_stats_object::voter >,
            member< peer_stats_object, float, &peer_stats_object::rank >,
            member< peer_stats_object, account_id_type, &peer_stats_object::peer >
         >,
         composite_key_compare< std::less<account_id_type>, std::greater<float>, std::less<account_id_type> >
      >,
      ordered_unique< tag< by_voter_peer >,
         composite_key< peer_stats_object,
            member< peer_stats_object, account_id_type, &peer_stats_object::voter >,
            member< peer_stats_object, account_id_type, &peer_stats_object::peer >
         >,
         composite_key_compare< std::less<account_id_type>,  std::less<account_id_type> >
      >
   >
> peer_stats_multi_index_type;
typedef graphene::db::generic_index< peer_stats_object, peer_stats_multi_index_type> peer_stats_index;




/**
 * Used to parse the metadata from the comment json_meta field.
 */
struct comment_metadata {
   set<string> tags;
};

/**
 *  This plugin will scan all changes to posts and/or their meta data and
 *
 */
class tags_plugin : public steemit::app::plugin
{
   public:
      tags_plugin( application* app );
      virtual ~tags_plugin();

      std::string plugin_name()const override;
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      friend class detail::tags_plugin_impl;
      std::unique_ptr<detail::tags_plugin_impl> my;
};

/**
 *  This API is used to query data maintained by the tags_plugin
 */
class tag_api : public std::enable_shared_from_this<tag_api> {
   public:
      tag_api(){};
      tag_api(const app::api_context& ctx){}//:_app(&ctx.app){}

      void on_api_startup(){
      }

      vector<tag_stats_object> get_tags()const { return vector<tag_stats_object>(); }

   private:
      //app::application* _app = nullptr;
};



} } //steemit::tag

FC_API( steemit::tags::tag_api, (get_tags) );

FC_REFLECT_DERIVED( steemit::tags::tag_object, (graphene::db::object),
    (tag)(created)(active)(cashout)(net_rshares)(net_votes)(hot)(promoted_balance)(children)(children_rshares2)(total_payout)(author)(parent)(comment) )

FC_REFLECT_DERIVED( steemit::tags::tag_stats_object, (graphene::db::object),
                    (tag)(total_children_rshares2)(total_payout)(net_votes)(top_posts)(comments) );

FC_REFLECT_DERIVED( steemit::tags::peer_stats_object, (graphene::db::object), (voter)(peer)(direct_positive_votes)(direct_votes)(indirect_positive_votes)(indirect_votes)(rank) );
FC_REFLECT( steemit::tags::comment_metadata, (tags) );
