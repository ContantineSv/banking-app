#include <iostream>
#include <vector>
#include <limits>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <errno.h>

#define PORT 8888

enum class Mode
{
	logged_off,
	logged_in,
	exit,
};

void clear_screen()
{
#ifdef _WIN32
    std::system("cls");
#else
    std::system ("clear");
#endif
}

void pressAnyKeyToContinue()
{
#ifdef _WIN32
    std::system("pause");
#else
    std::system ("read");
#endif
}

void printAccounts(const std::vector<int32_t>& accounts)
{
    for (const auto& account : accounts)
    {
        std:: cout << account << '\n';
    }
}

std::vector<int32_t> getAccounts(int socket)
{
    uint16_t n_accounts;
    std::vector<int32_t> accounts;
    recv(socket, reinterpret_cast<char*>(&n_accounts), sizeof(n_accounts), 0);
    n_accounts = ntohs(n_accounts);
    for (uint16_t i = 0; i < n_accounts; ++i)
    {
        int32_t account = 0;
        recv(socket, reinterpret_cast<char*>(&account), sizeof(account), 0);
        account = ntohl(account);
        accounts.push_back(account);
    }
    return accounts;
}

void session (int socket)
{   
    clear_screen();
    std::vector<int32_t> accounts; 
    Mode mode = Mode::logged_off;
    while(mode != Mode::exit)
    {   
        uint16_t option = 0;
        if (mode == Mode::logged_off)
        {
            while (option <= 0 || option >3)
            {
                std:: cout << "Welcom to the bank, chose your option" << '\n';
                std::cout << "1. Login" << '\n';
                std::cout << "2. Register" << '\n';
                std::cout << "3. Exit" << '\n';
                std::cin >> option;
                std::cin.clear();
                std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                clear_screen();
            }
            option = htons(option);
            send(socket, reinterpret_cast<char*>(&option), sizeof(option), 0);
            option = ntohs(option);
            switch (option)
            {
                case 1:
                {
                    char buff[20];
                    uint16_t response = -1;
                    std::cout << "Enter your login" << '\n';
                    std::cin >> buff;
                    send(socket, buff, 20, 0);
                    std::cout << "Enter your password" << '\n';
                    std::cin >> buff;
                    send(socket, buff, 20, 0);
                    recv(socket, reinterpret_cast<char*>(&response), sizeof(response), 0);
                    response = ntohs(response);
                    // get accounts if logged in and set the mode to logged in;
                    if (response == 0)
                    {
                        mode = Mode::logged_in;
                        std::cout << "Login successful\n";
                        accounts = getAccounts(socket);
                    }
                    else if (response == 1)
                    {
                        std::cout << "User  does not exist\n";
                    }
                    else if (response == 2)
                    {
                        std::cout << "Incorrect password ";
                    }
                    pressAnyKeyToContinue();
                    clear_screen();
                    break;
                }
                case 2:
                {
                    char buff[20];
                    uint16_t response = -1;
                    std::cout << "Enter a new login" << '\n';
                    std::cin >> buff;
                    send(socket, buff, 20, 0);
                    std::cout << "Enter a new password" << '\n';
                    std::cin >> buff;
                    send(socket, buff, 20, 0);
                    std::cout << "Confirm password" << '\n';
                    std::cin >> buff;
                    send(socket, buff, 20, 0);
                    recv(socket, reinterpret_cast<char*>(&response), sizeof(response), 0);
                    response = ntohs(response);
                    if (response == 0)
                    {
                        std::cout << "Registration successful ";
                    }
                    else if (response == 1)
                    {
                        std::cout << "The given user already exists ";
                    }
                    else if (response == 2)
                    {
                        std::cout << "Passwords don't match ";
                    }
                    pressAnyKeyToContinue();
                    clear_screen();
                    break;   
                }
                case 3:
                {   
                    clear_screen();
                    mode = Mode::exit;
                    break;
                }
            }   
        }
        else if (mode == Mode::logged_in)
        {
            std::cout << " Your account list:" << '\n';
            printAccounts(accounts);
            std::cout << "1. Exit" << '\n';
            std::cout << "2. Log out" << '\n';
            std::cout << "3. Add account" << '\n';
            std::cout << "4. Delete account" << '\n'; 
            while (option <=0 || option > 4)
            {
                std::cin >> option;
            }
            for (const auto& account: accounts)
            {
                std::cout << account << '\n';
            }
            option = htons(option);
            send(socket, reinterpret_cast<char*>(&option), sizeof(option), 0);
            option = ntohs(option);
            switch (option){
                // exit
                case 1:
                {
                    mode = Mode::exit;
                    break;
                }
                // login
                case 2:
                {
                    mode = Mode::logged_off;
                    break;
                }
                // add account
                case 3:
                {   
                    accounts = getAccounts(socket);
                    break;
                }
                // delete account
                case 4:
                {
                    std::cout << "Inter account number to delete ";
                    int32_t account_n;
                    std::cin >> account_n;
                    account_n = htonl(account_n);
                    send(socket, reinterpret_cast<char*>(&account_n), sizeof(account_n), 0);
                    uint16_t response = -1;
                    recv(socket, reinterpret_cast<char*>(&response), sizeof(response), 0);
                    if (response == 0)
                    {
                        std::cout << "The account has been deleted\n"; 
                    }
                    else
                    {
                        std::cout << "The account specified does not exist\n";
                    }
                    accounts = getAccounts(socket);
                    pressAnyKeyToContinue();
                    break;
                }
            }
            clear_screen();
        }
    }
}


int main(int argc, const char * argv[]) {

    if (argc < 2)
    {
        std::cout << "Specify host as an argument" << std::endl;
        exit(0);
    }

#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult !=  NO_ERROR)
    {
        std::cout << "WSAStartup failed with error:" << iResult << std::endl;
        exit(1);
    }
#endif

    sockaddr_in addr;
    int client;

    client = socket(AF_INET, SOCK_STREAM, 0);

    if (client < 0)
    {
        std::cerr << "Could not open a socket" << std::endl;
#ifdef _WIN32
        closesocket(client);
        WSACleanup();
#endif
        exit(1);
    }
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;

   if  (connect(client, (struct sockaddr*) &addr,  sizeof (addr)) < 0)
   {
       std::cerr << "Could not establish connection with the server" << std::endl;
#ifdef _WIN32
       closesocket(client);
       WSACleanup();
#endif
       exit(1);
   }

   session(client);

#ifdef _WIN32
    closesocket(client);
    WSACleanup();
#else
    close(client);
#endif

    return 0;
}