#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <contracts.hpp>
#include <utils.hpp>
#include <tables/config_table.hpp>
#include <tables/size_table.hpp>

using namespace eosio;
using std::string;

   /**
    * The cosale contract supports a collaborative sale of the tendered token (e.g. SEEDS).
    * Participants join the collaborative sale by sending tokens to the cosale contract in
    * one or more transactions. Participants are issued share-of-sale tokens
    * (e.g. COSALE) in return, 1:1 to the submitted tokens. For general participants these
    * share_tokens are freely transferrable tokens with the behavior of the standard
    * eosio.token contract.
    * The share-of-sale tokens have an additional feature that they automatically liquidate
    * into cash tokens (e.g. SEEDUSD), over time, as the tendered tokens are sold.
    * An account (e.g. an institutional account like milest.seeds) may be registered as
    * as special participant in the collaborative sale whose share_tokens are not
    * transferrable. A special participant may also be configured to have a sales
    * participation ratio different from the general participants.
    **/

CONTRACT cosale : public contract {

  public:

    using contract::contract;
    cosale(name receiver, name code, datastream<const char*> ds)
      : contract(receiver, code, ds)
        {}

     /**
          * This action clears RAM tables for the contract. For a large deployment,
          * attempting to erase all table entries in one action might fail by exceeding the
          * chain execution time limit. The `limit` parameter protects against this. It is
          * advisable for the application to check the contract status (get_scope) to
          * discover whether a follow-up `reset` action is required.
          *
          * @param limit - max number of erasures (for time control)
          *
          * @pre Transaction must have the contract account authority 
      */
      ACTION reset( const uint32_t limit );
         
      /**
          * The one-time `init` action executed by the cosale contract account records
          *  the managed tokens and the manager account in the config table.
          *
          * @param tender_token - the token being offered in the collaborative sale,
          * @param cash_token - the token which returns fiat value to the participants,
          * @param share_token - the token which represents the participant's share in the sale,
          * @param manager - an account empowered to execute administrative actions
      */
      ACTION init(extended_symbol tender_token, extended_symbol cash_token,
                  symbol share_token, name manager);
         
      /**
          * The `ontransfer` action watches for blockchain transfers and triggers
          * a response when 
          *  - tendered tokens are deposited to this contract
          *  - tendered tokens are sold from this contract
          *  - cash tokens are deposited to this contract
          *
          * @param from - the account sending tokens,
          * @param to - the account receiving tokens,
          * @param quantity - the amount transferred,
          * @param memo - memo field from the original transaction
      */
      [[eosio::on_notify("*::transfer")]]
      void ontransfer(name from, name to, asset quantity, string memo);

      /**
          * The `payouts` action executed by the manager account
          *   (1) distributes earnings from the sale, and
          *   (2) decrements each participant's balance of tendered tokens,
          * both in proportion to each participant's balance (weighted by
          * sales_ratio if applicable).
          *
          * @param payout - the total amount (cash tokens) being distributed,
          * @param sold - the total amount (tendered tokens) collaboratively sold
      */
      ACTION payouts(asset payout, asset sold);

      /**
          * The `payout1` action is a subordinate function to `payouts`, which
          * allows processing a large list of distribution transactions in
          * smaller chunks to accommodate blockchain performance limitations.
          *
          * @param payout - the total amount (cash tokens) being distributed,
          * @param sold - the total amount (tendered tokens) collaboratively sold
          * @param start - the start index of the first account in this chunk,
          * @param chunksize - the number of payout transactions in this chunk,
          * @param old_total_balance - TBD,
           
      */
      ACTION payout1(asset payout, asset sold, 
                     uint64_t start, uint64_t chunksize, int64_t old_total_balance);

      /**
          * The `returns` action executed by the manager account terminates the
          * collaborative sale and redeems all the outstanding share share tokens with
          * the unsold tendered tokens.
          *
      */
      ACTION returns();

      /**
          * The `return1` action is a subordinate function to `returns`, which
          * allows processing a large list of redemption transactions in
          * smaller chunks to accommodate blockchain performance limitations.
          *
          * @param start - the start index of the first account in this chunk,
          * @param chunksize - the number of transactions in this chunk,
          * @param old_total_balance - ,
           
      */
      ACTION return1(uint64_t start, uint64_t chunksize, int64_t old_total_balance);

      /**
          * The `transfer` action executed by the owner of a share_token balance
          * is used to send share_tokens from one account to another.
          *
          * @param from - the account sending tokens,
          * @param to - the account receiving tokens,
          * @param quantity - the amount transferred,
          * @param memo - memo field
      */
      ACTION transfer(name from, name to, asset quantity, const string& memo);

      /**
          * The `regspecial` action executed by the manager account
          * registers a nontransferrable cosale participant. Subsequently,
          * the participant will be unable to transfer share_tokens. Also,
          * subsequent payout action will apply a weighting factor to
          * the account's participation in the sale.
          *
          * @param account - the account name,
          * @param sales_ratio - the proportion at which this account participates
          *                         in the collaborative sale,
          * @param memo - memo field
      */
      ACTION regspecial(name account, float32 sales_ratio, const string& memo);

      /**
          * The `unregspecial` action executed by the manager account removes
          * the special treatment of an account and places it in the general
          * participant category.
          *
          * @param account - the account name,
          * @param memo - memo field
      */
      ACTION unregspecial(name account, const string& memo);


  private:

    void send_transfer(const name & to, const asset & quantity, const string & memo);
    void update_share_token( const name& owner, const asset& quantity);
    void add_balance( const name& owner, const asset& value, const name& ram_payer );
    bool sub_balance( const name& owner, const asset& value );

    TABLE currency_stats {  // scoped on share_token symbol code
       asset    supply;
       asset    max_supply;
       name     issuer;

       uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };

    TABLE account {  // scoped on account name
      asset    balance;

      uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    TABLE cosale_config {  // singleton, scoped on contract
       name            withdrawal_mgr;
       extended_symbol tender_token;
       extended_symbol cash_token;
       symbol          share_token
    };

    TABLE special {  // singleton, scoped on account name
       float32     sales_ratio
    };

    typedef eosio::multi_index< "accounts"_n, account > accounts;
    typedef eosio::multi_index< "stat"_n, currency_stats > stats;
    typedef eosio::singleton< "configs"_n, cosale_config > configs;
    typedef eosio::multi_index< "configs"_n, cosale_config >  dump_for_config;
    typedef eosio::singleton< "specials"_n, special > specials;
    typedef eosio::multi_index< "specials"_n, special >  dump_for_special;

};

