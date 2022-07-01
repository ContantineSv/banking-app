#include "database.h"
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

void Database::getUsers(string file)
{   
    // open the data file
    ifstream input;
    input.open(file);
    if (!input.is_open())
    {
        cout << "Could not initialize users ... terminating" << endl;
        exit (1);
    }

    // read user id offset
    if (!input.eof())
        input.read((char*) &user_id_offset, sizeof(int32_t));
    else
        return;

    // read users
    User user;
    while(!input.eof())
    {
        input.read((char*) &user , sizeof(User));

        if (input.eof())
            break;

        users[user.id] = user;
        login_to_user[user.login] = user.id;
        cout << " LOGIN: " << user.login << " USER_ID" << user.id << endl;
    }
    input.close();
}

void Database::getAccounts(string file)
{
    ifstream input;
    input.open(file, ios::in | ios::binary);
    if (!input.is_open())
    {
        cout << "Could not initialize accounts ... terminating" << endl;
        exit(1);
    }

    // read account id offset
    if (!input.eof())
        input.read((char*) &account_id_offset, sizeof(int32_t));
    else
        return;

    Account account;
    while(!input.eof())
    {
        input.read((char*) &account, sizeof(Account));

        if (input.eof())
            break;

        accounts[account.id] = account;
        user_to_accounts[account.owner].insert(account.id);
    }
    input.close();
}

void Database::initialize(string users_data, string accounts_data)
{
    getUsers(users_data);
    getAccounts(accounts_data);
}

void Database::storeUsers(string data)
{
    ofstream output;
    output.open(data, ios::out | ios::binary);
    if (!output.is_open())
    {
        cout << "Could not store users" << endl;
    }
    output.write(reinterpret_cast<char*> (&user_id_offset), sizeof(int32_t));

    for (auto& [id, user]: users)
    {   
        output.write(reinterpret_cast<char*> (&user), sizeof(User));
    }
    output.close();
}

void Database::storeAccounts(string data)
{
    ofstream output;
    output.open(data, ios::out | ios::binary);
    if (!output.is_open())
    {
        cout << "Could not store accounts" << endl;
    }
    output.write(reinterpret_cast<char*> (&account_id_offset), sizeof(int32_t));

    for (auto& [id, account]: accounts)
    {
        output.write(reinterpret_cast<char*> (&account), sizeof(Account));
    }
    output.close();
}

void Database::store(string users_data, string accounts_data)
{
    storeUsers(users_data);
    storeAccounts(accounts_data);
}


uint16_t Database::addUser(string log, string pass, string confirm)
{   
    // check if passwords match
    if (pass != confirm)
        return 2;
    if (login_to_user.count(log))
        return 1;

    User user;
    user.id = user_id_offset++;


    strcpy(user.login, log.c_str());

    strcpy(user.password, log.c_str());

    users[user.id] = user;
    login_to_user[user.login] = user.id;
    return 0;
}

uint16_t Database::addAccount(int32_t user_id, Type type)
{   
    if (!users.count(user_id))
        return 1;
    Account account{account_id_offset, user_id, type, 0};
    accounts[account.id] = account;
    user_to_accounts[user_id].insert(account.id);
    return account_id_offset++;
}

uint16_t Database::closeAccount(int32_t account_id)
{
    if (!accounts.count(account_id))
        return 1;
    user_to_accounts.at(accounts[account_id].owner).erase(account_id);
    accounts.erase(account_id);
    return 0;
}

uint16_t Database::deposit(int32_t account_id, int32_t ammount)
{
    if (!accounts.count(account_id))
        return 1;
    accounts[account_id].balance += ammount;
    return 0;
}

uint16_t Database::withdraw(int32_t account_id, int32_t ammount)
{
    if (!accounts.count(account_id))
        return 1;
    if (accounts[account_id].balance < ammount)
        return 2;
    accounts[account_id].balance -= ammount;
    return 0;
}

uint16_t Database::transfer(int32_t origin_id, int32_t destination_id, int32_t amount)
{
    if (!accounts.count(origin_id))
        return 1;
    if (!accounts.count(destination_id))
        return 2;

    if (accounts[origin_id].balance < amount)
        return 3;
    
    accounts[origin_id].balance -= amount;
    accounts[destination_id].balance += amount;
    return 0;
}

void Database::displayUsers() const
{
    for (const auto& [id, user]: users)
    {
        cout << "USER ID: " << id << endl;
    }
    cout << endl;
}

void Database::displayUserAccounts(int32_t user_id) const
{
    if (!users.count(user_id))
        return;

    cout << "USER ID: " << user_id << endl;
    cout << "ACCOUNTS:" << endl;

    if (!user_to_accounts.count(user_id))
        return;

    for (const auto& account_id: user_to_accounts.at(user_id))
    {   
        Account account = accounts.at(account_id);
        cout << "ACCOUNT ID: " << account.id << " ";
        cout << "BALANCE: " << account.balance << endl;

    }
}

void Database::displayAccounts() const
{
    for (const auto& [key, user]: users)
    {
        displayUserAccounts(key);
        cout << endl;
    }
}
