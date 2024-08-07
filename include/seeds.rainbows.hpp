#pragma once

#include <eosio/asset.hpp>
#include <eosio/binary_extension.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>

#include <string>



using namespace eosio;

   using std::string;

   /**
    * The `rainbows` contract implements the functionality described in the design document
    * https://rieki-cordon.medium.com/1fb713efd9b1 .
    * In the development process we are building on the eosio.token code. 
    *
    * The token contract defines the structures and actions that allow users to create, issue, and manage
    * tokens for EOSIO based blockchains.
    * 
    * The `rainbows` contract class also implements a public static method: `get_balance`. This allows
    * one to check the balance of a token for a specified account.
    * 
    * The `rainbows` contract manages the set of tokens, backings, accounts and their corresponding balances,
    * by using four internal tables: the `accounts`, `stats`, `configs`, and `backings`. The `accounts`
    * multi-index table holds, for each row, instances of `account` object and the `account` object
    * holds information about the balance of one token. The `accounts` table is scoped to an eosio
    * account, and it keeps the rows indexed based on the token's symbol.  This means that when one
    * queries the `accounts` multi-index table for an account name the result is all the tokens under
    * this contract that account holds at the moment.
    * 
    * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each
    * row, which contains information about current supply, maximum supply, and the creator account.
    * The `stats` table is scoped to the token symbol_code. Therefore, when one queries the `stats`
    * table for a token symbol the result is one single entry/row corresponding to the queried symbol
    * token if it was previously created, or nothing, otherwise.
    *
    * The first two tables (`accounts` and `stats`) are structured identically to the `eosio.token`
    * tables, making "rainbow tokens" compatible with most EOSIO wallet and block explorer applications.
    * The two remaining tables (`configs` and `backings`) provide additional data specific to the rainbow
    * token.
    *
    * The `configs` singleton table contains names of administration accounts (e.g. membership_mgr,
    * freeze_mgr) and some configuration flags. The `configs` table is scoped to the token symbol_code
    * and has a single row per scope.
    *
    * The `backings` table contains backing relationships (backing currency, backing ratio, escrow account).
    * It is scoped by the token symbol_code and may contain 1 or more rows. It has a secondary index
    * based on the backing currency type.
    *
    * In addition, the `displays` singleton table contains json metadata intended for applications
    * (e.g. wallets) to use in UI display, such as a logo symbol url. It is scoped by token symbol_code.
    *
    * The `symbols` table is a housekeeping list of all the tokens managed by the contract. It is
    * scoped to the contract.
    */

   CONTRACT rainbows : public contract {
      public:
         using contract::contract;
         rainbows( name receiver, name code, datastream<const char*> ds )
         : contract( receiver, code, ds ),
         symboltable( receiver, receiver.value )
         {}

         /**
          * The ` create` action allows `issuer` account to create or reconfigure a token with the
          * specified characteristics. 
          * If the token does not exist, a new row in the stats table for token symbol scope is created
          * with the specified characteristics. At creation, its' approval flag is false, preventing
          * tokens from being issued.
          * If a token of this symbol does exist and update is permitted, the characteristics are updated.
          *
          * @param issuer - the account that creates the token,
          * @param maximum_supply - the maximum supply set for the token,
          * @param withdrawal_mgr - the account with authority to withdraw tokens from any account,
          * @param withdraw_to - the account to which withdrawn tokens are deposited,
          * @param freeze_mgr - the account with authority to freeze transfer actions,
          * @param redeem_locked_until - an ISO8601 date string; user redemption of backings is
          *   disallowed until this time; blank string is equivalent to "now" (i.e. unlocked).
          * @param config_locked_until - an ISO8601 date string; changes to token characteristics
          *   are disallowed until this time; blank string is equivalent to "now" (i.e. unlocked).
          * @param membership_symbol - a frozen "sister" token, also managed by this contract;
          *   a balance of 1 or 2 sister tokens classifies an account as "visitor" or "regular" member
          * @param broker_symbol - a frozen "sister" token, also managed by this contract;
          *   a balance of the sister token qualifies an account as holding the "broker" badge
          * @param cred_limit_symbol - a frozen "sister" token, also managed by this contract;
          *   a positive balance in the sister token will permit a user to overspend to that amount
          * @param pos_limit_symbol - a frozen "sister" token, also managed by this contract;
          *   no user transfer is allowed to increase the user balance over the sister token balance.
          * @param valuation_mgr - the account with authority to set valuation.
          *
          * @pre Token symbol has to be valid,
          * @pre Token symbol must not be already created, OR if it has been created,
          *   the config_locked field in the configtable row must be in the past,
          * @pre maximum_supply has to be smaller than the maximum supply allowed by the system: 2^62 - 1.
          * @pre Maximum supply must be positive,
          * @pre withdrawal manager must be an existing account,
          * @pre withdraw_to must be an existing account,
          * @pre freeze manager must be an existing account,
          * @pre redeem_locked_until must specify a time within +100/-10 yrs of now;
          * @pre config_locked_until must specify a time within +100/-10 yrs of now;
          * @pre membership_symbol must be an existing frozen token of zero precision on this contract, or empty
          * @pre broker_symbol must be an existing frozen token of zero precision on this contract, or empty
          * @pre cred_limit_symbol must be an existing frozen token of matching precision on this contract, or empty
          * @pre pos_limit_symbol must be an existing frozen token of matching precision on this contract, or empty
          
          */
         ACTION create( const name&   issuer,
                        const asset&  maximum_supply,
                        const name&   withdrawal_mgr,
                        const name&   withdraw_to,
                        const name&   freeze_mgr,
                        const string& redeem_locked_until,
                        const string& config_locked_until,
                        const string& membership_symbol,
                        const string& broker_symbol,
                        const string& cred_limit_symbol,
                        const string& pos_limit_symbol,
                        const binary_extension<name>& valuation_mgr );


         /**
          * By this action the contract owner approves or rejects the creation of the token. Until
          * this approval, no tokens may be issued. If rejected, and no issued tokens are outstanding,
          * the table entries for this token are deleted.
          *
          * @param symbolcode - the symbol_code of the token to execute the close action for.
          * @param reject_and_clear - if this flag is true, delete token; if false, approve creation
          *
          * @pre The symbol must have been created.
          */
         ACTION approve( const symbol_code& symbolcode, const bool& reject_and_clear );

        /**
          * Allows `valuation_mgr` account to assign a valuation of the token with
          * reference to another currency (e.g. USD, EUR)
          *
          * Note: the static function `get_valuation` returns valuation (e.g. USD per token) 
          *
          * @param symbolcode - the token symbol
          * @param val_per_token - the quantity (float32) of a reference currency which is
          *                       considered equal in value one token
          * @param ref_currency - a string specifying the reference currency
          *                       Most commonly this will be an ISO 4217 code, however
          *                       interpretation of this string is the responsibility
          *                       of a wallet or other app, not this contract.
          * @param memo - memo string
          *
          */
         ACTION setvaluation( const symbol_code& symbolcode,
                              const float& val_per_token,
                              const string& ref_currency,
                              const string& memo );

        /**
          * Read the valuation (in the configured ref_currency) for a specified
          * quantity of tokens, based on the config parameters submitted in an
          * earlier `setvaluation` action.
          *
          * @param amount - the quantity of tokens
          *
          * @return - a 2-element structure containing
          *           currency: the ref_currency designator (typ an ISO 4217 code)
          *           valuation: a floating point quantity
          */
         struct valuation_t { string currency; float valuation; };

         [[eosio::action]] valuation_t getvaluation( const asset& amount ) {
           valuation_t rv = get_valuation( get_self(), amount.symbol.code() );
           rv.valuation *= amount.amount / pow(10, amount.symbol.precision());
           return rv;
         }

         /**
          * Allows `issuer` account to create a backing relationship for a token. A new row in the
          * backings table for token symbol scope gets created with the specified characteristics.
          *
          * @param token_bucket - a reference quantity of the token,
          * @param backs_per_bucket - the number of backing tokens (e.g. Seeds) placed in escrow per "bucket" of tokens,
          * @param backing_token_contract - the backing token contract account (e.g. token.seeds),
          * @param escrow - the escrow account where backing tokens are held
          * @param proportional - redeem by proportion of escrow rather than by backing ratio.
          * @param reserve_fraction - minimum reserve ratio (as percent) of escrow balance to redemption liability.
          * @param memo - the memo string to accompany the transaction.
          *
          * @pre Token symbol must have already been created by this issuer
          * @pre The config_locked_until field in the configs table must be in the past,
          * @pre issuer must have a (possibly zero) balance of the backing token,
          * @pre backs_per_bucket must be non-negative
          * @param reserve_fraction must be non-negative
          * @pre issuer active permissions must include rainbowcontract@eosio.code
          * @pre escrow active permissions must include rainbowcontract@eosio.code
          *
          * Note: the contract cannot internally check the required permissions status
          */
         ACTION setbacking( const asset&      token_bucket,
                          const asset&      backs_per_bucket,
                          const name&       backing_token_contract,
                          const name&       escrow,
                          const bool&       proportional,
                          const uint32_t&   reserve_fraction,
                          const string&     memo);

         /**
          * Allows `issuer` account to delete a backing relationship. Backing tokens are returned
          * to the issuer account. The row is removed from the backings table.
          *
          * @param backing_index - the index field in the `backings` table
          * @param symbolcode - the backing token
          * @param memo - memo string
          *
          * @pre the config_locked_until field in the configs table must be in the past
          */
         ACTION deletebacking( const uint64_t& backing_index,
                             const symbol_code& symbolcode,
                             const string& memo );


         /**
          * Allows `issuer` account to create or update display metadata for a token.
          * Issuer pays for RAM.
          * The currency_display table is intended for apps to access (e.g. via nodeos chain API).
          *
          * @param symbolcode - the token,
          * @param json_meta - json string of metadata. Minimum expected fields are
          *      name - human friendly name of token, max 32 char
          *      logo - url pointing to a small png or gif image (typ. 128x128 with transparency)
          *   Recommended fields are
          *      logo_lg - url pointing to a larger png or gif image (typ. 1024 x 1024)
          *      web_link - url pointing to a web page describing the token & application
          *      background - url pointing to a png or gif image intended as a UI background
          *                     (e.g. as used in Seeds Light Wallet)
          *   Additional fields are permitted within the overal length limit: max 2048 chars.
          *
          * @pre Token symbol must have already been created by this issuer
          * @pre String parameters must be within length limits
          */
         ACTION setdisplay( const symbol_code&  symbolcode,
                            const string&       json_meta
         );

         /**
          *  This action issues a `quantity` of tokens to the issuer account, and transfers
          *  a proportional amount of backing tokens to escrow if backing is configured.
          *
          * @param quantity - the amount of tokens to be issued,
          * @memo - the memo string that accompanies the token issue transaction.
          * 
          * @pre The `approve` action must have been executed for this token symbol
          */
         ACTION issue( const asset& quantity, const string& memo );

         /**
          * The opposite for issue action, if all validations succeed,
          * it debits the statstable.supply amount. If `do_redeem` flag is true,
          * any backing tokens are released from escrow in proportion to the
          * quantity of tokens retired.
          *
          * @param owner - the account containing tokens to retire,
          * @param quantity - the quantity of tokens to retire,
          * @param do_redeem - if true, send backing tokens to owner,
          *                    if false, they remain in escrow,
          * @param memo - the memo string to accompany the transaction.
          *
          * @pre the redeem_locked_until configuration must be in the past (except that
          *   this action is always permitted to the issuer.)
          * @pre If any backing relationships exist, for each relationship :
          *   1. the proportional redemption flag must be configured true, OR
          *   2. the balance in the escrow account must meet the reserve_fraction criterion
          */
         ACTION retire( const name& owner, const asset& quantity,
                        const bool& do_redeem, const string& memo );

         /**
          * Allows `from` account to transfer to `to` account the `quantity` tokens.
          * One account is debited and the other is credited with quantity tokens.
          *
          * @param from - the account to transfer from,
          * @param to - the account to be transferred to,
          * @param quantity - the quantity of tokens to be transferred,
          * @param memo - the memo string to accompany the transaction.
          * 
          * @pre The transfers_frozen flag in the configs table must be false, except for
          *   administrative-account transfers
          * @pre If configured with a membership_symbol in `create` operation, the sender and
          *   receiver must both be members, and at least one of them must be a regular member
          * @pre The `from` account balance must be sufficient for the transfer (allowing for
          *   credit if configured with credit_limit_symbol in `create` operation)
          * @pre If configured with positive_limit_symbol in `create` operation, the transfer
          *   must not put the `to` account over its maximum limit
          */
         ACTION transfer( const name&    from,
                          const name&    to,
                          const asset&   quantity,
                          const string&  memo );
         
         /**
          * Allows `from` account to transfer to `to` account a fraction of its balance.
          * One account is debited and the other is credited.
          * The transaction must be signed by the withdrawal_mgr and the
          * `to` account must be the `withdraw_to` account.
          * This function is suitable for demurrage or wealth taxation.
          * When `from` balance is negative (e.g. mutual credit) nothing is transferred.
          * Fractions are expressed in parts per million (ppm).
          * For demurrage, the fraction withdrawn is proportional to the time elapsed since
          * the previous demurrage withdrawal. The rate is expressed as `ppm_per_week`.
          * For wealth taxation, the tax rate is expressed as `ppm_abs`.
          * The first time a demurrage action is applied to a particular account/token,
          * the date is registered but no transfer is made.
          *
          * @param from - the account to transfer from,
          * @param to - the account to be transferred to,
          * @param symbolcode - the token symbol,
          * @param ppm_per_week - the demurrage rate in ppm per week,
          * @param ppm_abs - the tax rate (in ppm),
          * @param memo - the memo string to accompany the transaction.
          * 
          * @pre the transaction must be authorized by the withrawal_mgr account
          * @pre The `to` account must be the withdraw_to account
          * @pre If configured with a membership_symbol in `create` operation, the sender and
          *   receiver must both be members, and at least one of them must be a regular member
          * @pre The `from` account balance must be sufficient for the transfer (allowing for
          *   credit if configured with credit_limit_symbol in `create` operation)
          * @pre If configured with positive_limit_symbol in `create` operation, the transfer
          *   must not put the `to` account over its maximum limit
          */
         ACTION garner( const name&        from,
                        const name&        to,
                        const symbol_code& symbolcode,
                        const int64_t&     ppm_per_week,
                        const int64_t&     ppm_abs,
                        const string&      memo );
         
         /**
          * Allows `ram_payer` to create an account `owner` with zero balance for
          * token `symbolcode` at the expense of `ram_payer`.
          *
          * @param owner - the account to be created,
          * @param symbolcode - the token symbol,
          * @param ram_payer - the account that supports the cost of this action.
          *
          * More information can be read [here](https://github.com/EOSIO/eosio.contracts/issues/62)
          * and [here](https://github.com/EOSIO/eosio.contracts/issues/61).
          */
         ACTION open( const name& owner, const symbol_code& symbolcode, const name& ram_payer );

         /**
          * This action is the opposite for open, it closes the account `owner`
          * for token `symbol`.
          *
          * @param owner - the owner account to execute the close action for,
          * @param symbolcode - the symbol of the token to execute the close action for.
          *
          * @pre The pair of owner plus symbol has to exist otherwise no action is executed,
          * @pre If the pair of owner plus symbol exists, the balance has to be zero.
          */
         ACTION close( const name& owner, const symbol_code& symbolcode );

         /**
          * This action freezes or unfreezes transaction processing
          * for token `symbol`.
          *
          * @param symbol - the symbol of the token to execute the freeze action for.
          * @param freeze - boolean, true = freeze, false = enable transfers.
          * @param memo - the memo string to accompany the transaction.
          *
          * @pre The symbol has to exist otherwise no action is executed,
          * @pre Transaction must have the freeze_mgr authority 
          */
         ACTION freeze( const symbol_code& symbolcode, const bool& freeze, const string& memo );

         /**
          * This action clears RAM tables for all tokens. For a large deployment,
          * attempting to erase all table entries in one action might fail by exceeding the
          * chain execution time limit. The `limit` parameter protects against this. It is
          * advisable for the application to check the contract status (get_scope) to
          * discover whether a follow-up `reset` action is required.
          *
          * @param all - if true, clear all tables within the token scope;
          *              if false, keep accounts, stats, and symbols
          * @param limit - max number of erasures (for time control)
          *
          * @pre Transaction must have the contract account authority 
          */
         ACTION reset( const bool all, const uint32_t limit );

         /**
          * This action clears the `accounts` table for a particular account. All
          * token balances in the account are erased.
          *
          * @param account - account
          *
          * @pre Transaction must have the contract account authority 
          */
         ACTION resetacct( const name& account );

         static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto ac = accountstable.find( sym_code.raw() );
            if( ac == accountstable.end() ) {
               return null_asset;
            }
            return ac->balance;
         }

         static valuation_t get_valuation( const name& token_contract_account, const symbol_code& sym_code )
         {
           configs configtable( token_contract_account, sym_code.raw() );
           check(configtable.exists(), "symbol does not exist");
           auto cf = configtable.get();
           if (!cf.ref_currency.has_value() || cf.ref_currency.value() == "") {
             return {"", 0.00};
           }
           return {cf.ref_currency.value(), cf.val_per_token.value()};
         }

      private:
         const int max_backings_count = 8; // don't use too much cpu time to complete transaction
         const uint64_t no_index = static_cast<uint64_t>(-1); // flag for nonexistent defer_table link
         static const asset null_asset;
         const uint32_t VISITOR = 1;
         const uint32_t REGULAR = 2;

         TABLE account { // scoped on account name
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         TABLE currency_stats {  // scoped on token symbol code
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         TABLE currency_config {  // singleton, scoped on token symbol code
            name        withdrawal_mgr;
            name        withdraw_to;
            name        freeze_mgr;
            time_point  redeem_locked_until;
            time_point  config_locked_until;
            bool        transfers_frozen;
            bool        approved;
            symbol_code membership;
            symbol_code broker;
            symbol_code cred_limit;
            symbol_code positive_limit;
            // binary_extension<> for backward compatibility
            binary_extension<name>
                        valuation_mgr;
            binary_extension<float>
                        val_per_token;
            binary_extension<string>
                        ref_currency;
         };

         TABLE currency_display {  // singleton, scoped on token symbol code
            string     json_meta;
         };


         TABLE backing_stats {  // scoped on token symbol code
            uint64_t index;
            asset    token_bucket;
            asset    backs_per_bucket;
            name     backing_token_contract;
            name     escrow;
            uint32_t reserve_fraction;
            bool     proportional;

            uint64_t primary_key()const { return index; };
            uint128_t by_secondary() const {
               return (uint128_t)backs_per_bucket.symbol.raw()<<64 |
                        backing_token_contract.value;
            }
         };

         TABLE symbolt { // scoped on get_self()
            symbol_code  symbolcode;

            uint64_t primary_key()const { return symbolcode.raw(); };
         };

         TABLE garner_dates { // scoped on symbolcode
            name  account;
            time_point last_garner;

            uint64_t primary_key()const { return account.value; };
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;
         typedef eosio::singleton< "configs"_n, currency_config > configs;
         typedef eosio::multi_index< "configs"_n, currency_config >  dump_for_config;
         typedef eosio::singleton< "displays"_n, currency_display > displays;
         typedef eosio::multi_index< "displays"_n, currency_display >  dump_for_display;
         typedef eosio::multi_index< "backings"_n, backing_stats, indexed_by
               < "backingtoken"_n,
                 const_mem_fun<backing_stats, uint128_t, &backing_stats::by_secondary >
               >
            > backs;
         typedef eosio::multi_index< "symbols"_n, symbolt > symbols;
         symbols symboltable;
         typedef eosio::multi_index< "garnerdates"_n, garner_dates > garnerdates;

         void sub_balance( const name& owner, const asset& value, const symbol_code& limit_symbol );
         void add_balance( const name& owner, const asset& value, const name& ram_payer,
                           const symbol_code& limit_symbol );
         void sister_check(const string& sym_name, uint32_t precision);
         void set_all_backings( const name& owner, const asset& quantity );
         void redeem_all_backings( const name& owner, const asset& quantity );
         void set_one_backing( const backing_stats& bk, const name& owner, const asset& quantity );
         void redeem_one_backing( const backing_stats& bk, const name& owner, const asset& quantity );
         void reset_one( const symbol_code symbolcode, const bool all, const uint32_t limit, uint32_t& counter );
 
   };

