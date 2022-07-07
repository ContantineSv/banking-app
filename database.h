#pragma once
#include <map>
#include <vector>
#include <set>
#include <string>
#include <mutex>

class Database
{
public:
    static Database& getInstance()
    {
        static Database instance;
        return instance;
    }

    Database(Database const&) = delete;
    void operator=(Database const&)  = delete;

    // get data from database
    void initialize(std::string users_data, std::string accounts_data);
    // store data to database
    void store(std::string user_data, std::string account_data);
    // return 0 if Ok, 1 if user already exists,
    // 2 if passwords dont match
    uint16_t addUser(std::string log, std::string pass , std::string confirm);
    // returns account number if OK and -1 if user doesn't exist
    uint16_t addAccount(int32_t user_id);
    // returns 1 if OK and 0 if doesn't exist;
    uint16_t closeAccount(int32_t account_id);
    // returns 0 if OK and 1 if account doesn't exist
    uint16_t deposit(int32_t account_id, int32_t ammount);
    // returns 0 if OK, 1 if doesn't exist, 2 if not enough funds
    uint16_t withdraw(int32_t account_id, int32_t ammount);
    // return 0 if OK, 1 if origin account doesn't exist,
    // 2 if destination account doesn't exist, 3 if not enough funds
    uint16_t transfer(int origin_id, int destination_id, int ammount);

    void displayUsers() const;
    void displayUserAccounts(int user_id) const;
    void displayAccounts() const;

    // return -1 if fail
    int32_t getUserByLogin(std::string login) const
    {
        std::lock_guard<std::mutex> l(_mtx);
        if (!login_to_user.count(login))
            return -1;
        return login_to_user.at(login);
    }

    // 0 - ok , 1 - user does not exist, 2 - invalid password
    uint16_t login(std::string login, std::string password) const
    {
        std::lock_guard<std::mutex> l(_mtx);
        if (login_to_user.count(login) == 0)
            return 1;
        if (users.at(login_to_user.at(login)).password != password)
            return 2;
        else return 0;
    }
    // get account numbers for an user
    std::vector<int32_t> getUsersAccounts(int32_t user_id) const
    {
        std::lock_guard<std::mutex> l(_mtx);
        if (!user_to_accounts.count(user_id))
            return std::vector<int32_t>();
        std::set<int32_t> accounts = user_to_accounts.at(user_id);
        return std::vector<int32_t> (accounts.begin(), accounts.end());
    }



private:
    Database() = default;

    struct User
    {
        int32_t id;
        char login[20];
        char password[20];
    };

    struct Account
    {
        int32_t id;
        int32_t owner;
        int32_t balance; 
    };

    void getUsers(std::string data);
    void getAccounts(std::string data);
    void storeUsers(std::string data);
    void storeAccounts(std::string data);

    std::map<int32_t, User> users;
    std::map<int32_t, Account> accounts;
    std::map<int32_t, std::set<int32_t>> user_to_accounts;
    std::map <std::string, int32_t> login_to_user;
    int32_t user_id_offset = 0;
    int32_t account_id_offset = 0;

    mutable std::mutex _mtx; 
};