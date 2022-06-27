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
    * one or more transactions. In return, the participant is issued share-of-sale tokens
    * (e.g. COSALE) 1:1 to the submitted tokens. These are transferrable tokens based on
    * the standard eosio.token contract.
    * The share-of-sale tokens have an additional feature that they automatically liquidate
    * into a cash token (e.g. SEEDUSD) as the tendered tokens are sold.
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
      ACTION reset();
         
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
          * The `payouts` action executed by the manager account distributes
          *  earnings from the sale in proportion to participant balances.
          *
          * @param quantity - the total amount (cash tokens) being distributed,
      */
      ACTION payouts(asset quantity);

      /**
          * The `payout1` action is a subordinate function to `payouts`, which
          * allows processing a large list of distribution transactions in
          * smaller chunks to accommodate blockchain performance limitations.
          *
          * @param quantity - the total amount (cash tokens) being distributed,
          * @param start - the start index of the first account in this chunk,
          * @param chunksize - the number of payout transactions in this chunk,
          * @param old_total_balance - ,
           
      */
      ACTION payout1(asset quantity, uint64_t start, uint64_t chunksize, int64_t old_total_balance);

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

    TABLE account {
      asset    balance;

      uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    TABLE cosale_config {  // singleton, scoped on token symbol code
       name            withdrawal_mgr;
       extended_symbol tender_token;
       extended_symbol cash_token;
       symbol share_token
    };

    typedef eosio::multi_index< "accounts"_n, account > accounts;
    typedef eosio::multi_index< "stat"_n, currency_stats > stats;
    typedef eosio::singleton< "configs"_n, cosale_config > configs;
    typedef eosio::multi_index< "configs"_n, cosale_config >  dump_for_config;

};

