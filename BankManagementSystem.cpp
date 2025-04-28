#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <cctype>
#include <memory>
#include <queue>
using namespace std;

class Account {
protected:
    int accountNumber;
    string name;
    double balance;
    char type;
public:
    Account(int accNo, const string& accName, double bal, char accType)
        : accountNumber(accNo), name(accName), balance(bal), type(accType) {}
    virtual ~Account() = default;

    int getAccountNumber() const { return accountNumber; }
    string getName() const { return name; }
    double getBalance() const { return balance; }
    char getType() const { return type; }

    void setName(const string& newName) { name = newName; }
    void setBalance(double newBal) { balance = newBal; }

    virtual void deposit(double amt) = 0;
    virtual bool withdraw(double amt) = 0;
    virtual void show() const = 0;
    virtual void serialize(ostream& out) const = 0;
};

class Savings : public Account {
    double minimumBalance;
public:
    Savings(int accNo, const string& accName, double bal, double minBal)
        : Account(accNo, accName, bal, 'S'), minimumBalance(minBal) {}

    void deposit(double amt) override { balance += amt; }
    bool withdraw(double amt) override {
        if (balance - amt < minimumBalance) return false;
        balance -= amt;
        return true;
    }
    void show() const override {
        cout << left << setw(10) << accountNumber
             << setw(20) << name
             << setw(6) << type
             << fixed << setprecision(2) << balance
             << "  (MinBal=" << minimumBalance << ")\n";
    }
    void serialize(ostream& out) const override {
        out << type << ',' << accountNumber << ',' << name << ','
            << balance << ',' << minimumBalance << '\n';
    }
    void setMinimumBalance(double minBal) { minimumBalance = minBal; }
};

class Checking : public Account {
    double overdraftLimit;
public:
    Checking(int accNo, const string& accName, double bal, double overdraft)
        : Account(accNo, accName, bal, 'C'), overdraftLimit(overdraft) {}

    void deposit(double amt) override { balance += amt; }
    bool withdraw(double amt) override {
        if (balance + overdraftLimit < amt) return false;
        balance -= amt;
        return true;
    }
    void show() const override {
        cout << left << setw(10) << accountNumber
             << setw(20) << name
             << setw(6) << type
             << fixed << setprecision(2) << balance
             << "  (Overdraft=" << overdraftLimit << ")\n";
    }
    void serialize(ostream& out) const override {
        out << type << ',' << accountNumber << ',' << name << ','
            << balance << ',' << overdraftLimit << '\n';
    }
    void setOverdraftLimit(double od) { overdraftLimit = od; }
};

class Bank {
    vector<unique_ptr<Account>> accounts;
    const string filename = "accounts.txt";

    void loadFromFile() {
        ifstream in(filename);
        string line;
        while (getline(in, line)) {
            istringstream iss(line);
            char t, comma;
            int accNo;
            string accName;
            double bal, param;
            if (!(iss >> t >> comma >> accNo >> comma)) continue;
            if (!getline(iss, accName, ',')) continue;
            if (!(iss >> bal >> comma >> param)) continue;
            if (t == 'S') {
                accounts.push_back(make_unique<Savings>(accNo, accName, bal, param));
            } else if (t == 'C') {
                accounts.push_back(make_unique<Checking>(accNo, accName, bal, param));
            }
        }
    }

    void saveToFile() const {
        ofstream out(filename, ofstream::trunc);
        for (auto const& acc : accounts) acc->serialize(out);
    }

    Account* findAccount(int accno) {
        for (auto const& acc : accounts)
            if (acc->getAccountNumber() == accno)
                return acc.get();
        return nullptr;
    }

public:
    Bank() { loadFromFile(); }
    ~Bank() { saveToFile(); }

    void createAccount() {
        int accNo; string accName; double bal, param;
        char t;
        cout << "\nEnter Account Number: "; cin >> accNo;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter Account Holder Name: "; getline(cin, accName);
        cout << "Enter Account Type (S for Savings, C for Checking): "; cin >> t;
        t = toupper(t);
        cout << "Enter Initial Balance: "; cin >> bal;
        if (t == 'S') {
            cout << "Enter Minimum Balance: "; cin >> param;
            accounts.push_back(make_unique<Savings>(accNo, accName, bal, param));
        } else {
            cout << "Enter Overdraft Limit: "; cin >> param;
            accounts.push_back(make_unique<Checking>(accNo, accName, bal, param));
        }
        saveToFile();
        cout << "Account created successfully.\n";
    }

    void depositToAccount(int accno) {
        if (auto* acc = findAccount(accno)) {
            double amt;
            cout << "Enter amount to deposit: "; cin >> amt;
            acc->deposit(amt);
            saveToFile();
            cout << "Deposit successful.\n";
        } else cout << "Account not found.\n";
    }

    void withdrawFromAccount(int accno) {
        if (auto* acc = findAccount(accno)) {
            double amt;
            cout << "Enter amount to withdraw: "; cin >> amt;
            if (acc->withdraw(amt)) {
                saveToFile();
                cout << "Withdrawal successful.\n";
            } else cout << "Withdrawal failed (limits reached).\n";
        } else cout << "Account not found.\n";
    }

    void balanceEnquiry(int accno) const {
        for (auto const& acc : accounts) {
            if (acc->getAccountNumber() == accno) {
                cout << "\nAccount Details:\n";
                acc->show();
                return;
            }
        }
        cout << "Account not found.\n";
    }

    void listAllAccounts() const {
        cout << left << setw(10) << "AccNo"
             << setw(20) << "Name"
             << setw(6) << "Type"
             << "Balance (plus params)\n";
        cout << string(60, '-') << "\n";
        for (auto const& acc : accounts) acc->show();
    }

    void closeAccount(int accno) {
        auto it = remove_if(accounts.begin(), accounts.end(),
            [&](const unique_ptr<Account>& acc) {
                return acc->getAccountNumber() == accno; });
        if (it != accounts.end()) {
            accounts.erase(it, accounts.end());
            saveToFile();
            cout << "Account closed.\n";
        } else cout << "Account not found.\n";
    }

    void modifyAccount(int accno) {
        if (auto* acc = findAccount(accno)) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            string newName;
            double newBal, newParam;
            cout << "Enter new holder name: "; getline(cin, newName);
            cout << "Enter new balance: "; cin >> newBal;
            acc->setBalance(newBal);
            acc->setName(newName);
            if (acc->getType() == 'S') {
                auto* sav = dynamic_cast<Savings*>(acc);
                cout << "Enter new minimum balance: "; cin >> newParam;
                sav->setMinimumBalance(newParam);
            } else if (acc->getType() == 'C') {
                auto* chk = dynamic_cast<Checking*>(acc);
                cout << "Enter new overdraft limit: "; cin >> newParam;
                chk->setOverdraftLimit(newParam);
            }
            saveToFile();
            cout << "Account modified.\n";
        } else cout << "Account not found.\n";
    }

    void showTopAccounts(int k) const {
        if (accounts.empty()) { cout << "No accounts available.\n"; return; }
        // Correct comparator: max-heap based on balance
        auto cmp = [](Account* a, Account* b) { return a->getBalance() < b->getBalance(); };
        priority_queue<Account*, vector<Account*>, decltype(cmp)> pq(cmp);
        for (auto const& acc : accounts) pq.push(acc.get());
        int n = min(k, (int)pq.size());
        cout << "\nTop " << n << " Accounts by Balance:\n";
        cout << left << setw(10) << "AccNo"
             << setw(20) << "Name"
             << setw(6) << "Type"
             << "Balance\n";
        cout << string(50, '-') << "\n";
        for (int i = 0; i < n; ++i) {
            pq.top()->show();
            pq.pop();
        }
    }
};

int main() {
    Bank bank;
    char choice;
    int accno;
    do {
        system("CLS");
        cout << "\n===== BANK MANAGEMENT SYSTEM =====\n";
        cout << "1. New Account\n";
        cout << "2. Deposit Amount\n";
        cout << "3. Withdraw Amount\n";
        cout << "4. Balance Enquiry\n";
        cout << "5. All Accounts List\n";
        cout << "6. Close An Account\n";
        cout << "7. Modify An Account\n";
        cout << "8. Show Top Accounts by Balance\n";
        cout << "9. Exit\n";
        cout << "Select Your Option (1-9): "; cin >> choice;
        switch (choice) {
            case '1': bank.createAccount(); break;
            case '2': cout << "Enter account no: "; cin >> accno; bank.depositToAccount(accno); break;
            case '3': cout << "Enter account no: "; cin >> accno; bank.withdrawFromAccount(accno); break;
            case '4': cout << "Enter account no: "; cin >> accno; bank.balanceEnquiry(accno); break;
            case '5': bank.listAllAccounts(); break;
            case '6': cout << "Enter account no: "; cin >> accno; bank.closeAccount(accno); break;
            case '7': cout << "Enter account no: "; cin >> accno; bank.modifyAccount(accno); break;
            case '8': { int k; cout << "Enter number of top accounts to display: "; cin >> k; bank.showTopAccounts(k); break; }
            case '9': cout << "Exiting...\n"; break;
            default: cout << "Invalid option.";
        }
        cout << "\nPress Enter to continue...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cin.get();
    } while (choice != '9');
    return 0;
}
