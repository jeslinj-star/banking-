

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

// ==================== CONSTANTS ====================
#define MAX_ACCOUNTS 100
#define MAX_NAME_LENGTH 50
#define MIN_PIN 1000
#define MAX_PIN 9999
#define INTEREST_RATE 0.05f
#define STARTING_BALANCE 1000.0f
#define LOAN_AMOUNT 500.0f
#define ASSET_PURCHASE_AMOUNT 100.0f
#define DATA_FILE "accounts.dat"

// ==================== ENUMERATIONS ====================
typedef enum {
    CRYPTO = 0,
    GOLD,
    SILVER
} AssetType;

typedef enum {
    EUR = 0,
    GBP,
    INR
} CurrencyType;

typedef enum {
    SUCCESS = 0,
    ERROR_INSUFFICIENT_FUNDS,
    ERROR_INVALID_PIN,
    ERROR_ACCOUNT_EXISTS,
    ERROR_FILE_IO,
    ERROR_INVALID_INPUT
} ErrorCode;

// ==================== STRUCTURES ====================
typedef struct {
    char name[MAX_NAME_LENGTH];
    int pin;
    float balance;
    float loan;
    
    // Asset holdings
    struct {
        float crypto;
        float gold;
        float silver;
    } assets;
    
    // Foreign currency holdings
    struct {
        float eur;
        float gbp;
        float inr;
    } currencies;
} Account;

typedef struct {
    float crypto;
    float gold;
    float silver;
} MarketPrices;

typedef struct {
    float eur;
    float gbp;
    float inr;
} ExchangeRates;

// ==================== GLOBAL STATE ====================
static Account accounts[MAX_ACCOUNTS];
static int accountCount = 0;
static int currentUserIndex = -1;

static MarketPrices marketPrices = {150.0f, 60.0f, 25.0f};
static ExchangeRates exchangeRates = {1.10f, 1.27f, 0.012f};

// ==================== UTILITY FUNCTIONS ====================

/**
 * Clear input buffer to prevent scanf issues
 */
void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * Validate if string contains only alphabetic characters
 */
bool isAlphaString(const char *str) {
    if (str == NULL || *str == '\0') return false;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalpha((unsigned char)str[i])) {
            return false;
        }
    }
    return true;
}

/**
 * Validate PIN is within valid range
 */
bool isValidPIN(int pin) {
    return (pin >= MIN_PIN && pin <= MAX_PIN);
}

/**
 * Safe float input with validation
 */
bool getFloatInput(const char *prompt, float *value) {
    printf("%s", prompt);
    if (scanf("%f", value) != 1) {
        clearInputBuffer();
        return false;
    }
    clearInputBuffer();
    return true;
}

/**
 * Safe integer input with validation
 */
bool getIntInput(const char *prompt, int *value) {
    printf("%s", prompt);
    if (scanf("%d", value) != 1) {
        clearInputBuffer();
        return false;
    }
    clearInputBuffer();
    return true;
}

/**
 * Display error message based on error code
 */
void displayError(ErrorCode error) {
    switch (error) {
        case ERROR_INSUFFICIENT_FUNDS:
            printf("\n[ERROR] Insufficient funds for this transaction.\n");
            break;
        case ERROR_INVALID_PIN:
            printf("\n[ERROR] Invalid PIN entered.\n");
            break;
        case ERROR_ACCOUNT_EXISTS:
            printf("\n[ERROR] Account with this name or PIN already exists.\n");
            break;
        case ERROR_FILE_IO:
            printf("\n[ERROR] File operation failed.\n");
            break;
        case ERROR_INVALID_INPUT:
            printf("\n[ERROR] Invalid input provided.\n");
            break;
        default:
            printf("\n[ERROR] An unknown error occurred.\n");
    }
}

// ==================== FILE OPERATIONS ====================

/**
 * Save all accounts to persistent storage
 */
ErrorCode saveAccounts(void) {
    FILE *file = fopen(DATA_FILE, "wb");
    if (file == NULL) {
        return ERROR_FILE_IO;
    }
    
    // Write account count
    if (fwrite(&accountCount, sizeof(int), 1, file) != 1) {
        fclose(file);
        return ERROR_FILE_IO;
    }
    
    // Write all accounts
    if (fwrite(accounts, sizeof(Account), accountCount, file) != (size_t)accountCount) {
        fclose(file);
        return ERROR_FILE_IO;
    }
    
    fclose(file);
    return SUCCESS;
}

/**
 * Load accounts from persistent storage
 */
ErrorCode loadAccounts(void) {
    FILE *file = fopen(DATA_FILE, "rb");
    if (file == NULL) {
        return SUCCESS; // File doesn't exist yet - not an error
    }
    
    // Read account count
    if (fread(&accountCount, sizeof(int), 1, file) != 1) {
        fclose(file);
        return ERROR_FILE_IO;
    }
    
    // Read all accounts
    if (fread(accounts, sizeof(Account), accountCount, file) != (size_t)accountCount) {
        fclose(file);
        return ERROR_FILE_IO;
    }
    
    fclose(file);
    return SUCCESS;
}

// ==================== ACCOUNT MANAGEMENT ====================

/**
 * Check if account name or PIN already exists
 */
bool accountExists(const char *name, int pin) {
    for (int i = 0; i < accountCount; i++) {
        if (strcmp(accounts[i].name, name) == 0 || accounts[i].pin == pin) {
            return true;
        }
    }
    return false;
}

/**
 * Initialize a new account with default values
 */
void initializeAccount(Account *account, const char *name, int pin) {
    strncpy(account->name, name, MAX_NAME_LENGTH - 1);
    account->name[MAX_NAME_LENGTH - 1] = '\0';
    account->pin = pin;
    account->balance = STARTING_BALANCE;
    account->loan = 0.0f;
    
    account->assets.crypto = 0.0f;
    account->assets.gold = 0.0f;
    account->assets.silver = 0.0f;
    
    account->currencies.eur = 0.0f;
    account->currencies.gbp = 0.0f;
    account->currencies.inr = 0.0f;
}

/**
 * Create a new account
 */
ErrorCode createAccount(void) {
    if (accountCount >= MAX_ACCOUNTS) {
        printf("\n[ERROR] Maximum account limit reached.\n");
        return ERROR_INVALID_INPUT;
    }
    
    char name[MAX_NAME_LENGTH];
    int pin;
    
    // Get and validate name
    printf("\n=== CREATE NEW ACCOUNT ===\n");
    while (true) {
        printf("Enter name (alphabets only): ");
        if (scanf("%49s", name) != 1) {
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        
        if (isAlphaString(name)) {
            break;
        }
        printf("[ERROR] Name must contain only alphabetic characters.\n");
    }
    
    // Get and validate PIN
    while (true) {
        if (!getIntInput("Set 4-digit PIN (1000-9999): ", &pin)) {
            printf("[ERROR] Invalid PIN format.\n");
            continue;
        }
        
        if (isValidPIN(pin)) {
            break;
        }
        printf("[ERROR] PIN must be between 1000 and 9999.\n");
    }
    
    // Check for duplicates
    if (accountExists(name, pin)) {
        return ERROR_ACCOUNT_EXISTS;
    }
    
    // Create and save account
    initializeAccount(&accounts[accountCount], name, pin);
    accountCount++;
    
    ErrorCode result = saveAccounts();
    if (result == SUCCESS) {
        printf("\n[SUCCESS] Account created successfully!\n");
        printf("Starting balance: $%.2f\n", STARTING_BALANCE);
    }
    
    return result;
}

/**
 * Authenticate user login
 */
ErrorCode loginAccount(void) {
    char name[MAX_NAME_LENGTH];
    int pin;
    
    printf("\n=== LOGIN ===\n");
    printf("Enter name: ");
    if (scanf("%49s", name) != 1) {
        clearInputBuffer();
        return ERROR_INVALID_INPUT;
    }
    clearInputBuffer();
    
    if (!getIntInput("Enter PIN: ", &pin)) {
        return ERROR_INVALID_INPUT;
    }
    
    // Search for matching account
    for (int i = 0; i < accountCount; i++) {
        if (strcmp(accounts[i].name, name) == 0 && accounts[i].pin == pin) {
            currentUserIndex = i;
            printf("\n[SUCCESS] Welcome, %s!\n", name);
            return SUCCESS;
        }
    }
    
    printf("\n[ERROR] Login failed. Invalid credentials.\n");
    return ERROR_INVALID_PIN;
}

/**
 * Verify PIN for current user
 */
bool verifyPIN(void) {
    int pin;
    if (!getIntInput("Enter PIN for verification: ", &pin)) {
        return false;
    }
    return (pin == accounts[currentUserIndex].pin);
}

// ==================== MARKET OPERATIONS ====================

/**
 * Update market prices with realistic fluctuations
 */
void updateMarketPrices(void) {
    // Crypto: volatile (-15% to +20%)
    float cryptoChange = ((rand() % 35) - 15) / 100.0f;
    marketPrices.crypto *= (1.0f + cryptoChange);
    
    // Gold: stable (-5% to +10%)
    float goldChange = ((rand() % 15) - 5) / 100.0f;
    marketPrices.gold *= (1.0f + goldChange);
    
    // Silver: moderate (-10% to +15%)
    float silverChange = ((rand() % 25) - 10) / 100.0f;
    marketPrices.silver *= (1.0f + silverChange);
    
    printf("\n=== MARKET UPDATE ===\n");
    printf("Crypto:  $%.2f (%.2f%%)\n", marketPrices.crypto, cryptoChange * 100);
    printf("Gold:    $%.2f (%.2f%%)\n", marketPrices.gold, goldChange * 100);
    printf("Silver:  $%.2f (%.2f%%)\n", marketPrices.silver, silverChange * 100);
}

/**
 * Display current market prices
 */
void displayMarketPrices(void) {
    printf("\n=== CURRENT MARKET PRICES ===\n");
    printf("Cryptocurrency: $%.2f per unit\n", marketPrices.crypto);
    printf("Gold:           $%.2f per unit\n", marketPrices.gold);
    printf("Silver:         $%.2f per unit\n", marketPrices.silver);
    printf("============================\n");
}

// ==================== BANKING OPERATIONS ====================

/**
 * Handle cash deposit
 */
ErrorCode depositCash(float amount) {
    if (amount <= 0) {
        return ERROR_INVALID_INPUT;
    }
    
    accounts[currentUserIndex].balance += amount;
    printf("\n[SUCCESS] Deposited $%.2f\n", amount);
    printf("New balance: $%.2f\n", accounts[currentUserIndex].balance);
    
    return saveAccounts();
}

/**
 * Handle cash withdrawal
 */
ErrorCode withdrawCash(float amount) {
    if (amount <= 0) {
        return ERROR_INVALID_INPUT;
    }
    
    if (amount > accounts[currentUserIndex].balance) {
        return ERROR_INSUFFICIENT_FUNDS;
    }
    
    if (!verifyPIN()) {
        return ERROR_INVALID_PIN;
    }
    
    accounts[currentUserIndex].balance -= amount;
    printf("\n[SUCCESS] Withdrawn $%.2f\n", amount);
    printf("New balance: $%.2f\n", accounts[currentUserIndex].balance);
    
    return saveAccounts();
}

/**
 * Process cash transactions (deposit/withdraw)
 */
void processCashTransaction(void) {
    int choice;
    float amount;
    
    printf("\n=== CASH TRANSACTION ===\n");
    printf("1. Deposit\n");
    printf("2. Withdraw\n");
    
    if (!getIntInput("Choice: ", &choice)) {
        displayError(ERROR_INVALID_INPUT);
        return;
    }
    
    if (!getFloatInput("Enter amount: $", &amount)) {
        displayError(ERROR_INVALID_INPUT);
        return;
    }
    
    ErrorCode result;
    if (choice == 1) {
        result = depositCash(amount);
    } else if (choice == 2) {
        result = withdrawCash(amount);
    } else {
        displayError(ERROR_INVALID_INPUT);
        return;
    }
    
    if (result != SUCCESS) {
        displayError(result);
    }
}

/**
 * Purchase assets (crypto, gold, silver)
 */
void purchaseAsset(void) {
    Account *user = &accounts[currentUserIndex];
    
    if (user->balance < ASSET_PURCHASE_AMOUNT) {
        displayError(ERROR_INSUFFICIENT_FUNDS);
        return;
    }
    
    if (!verifyPIN()) {
        displayError(ERROR_INVALID_PIN);
        return;
    }
    
    printf("\n=== PURCHASE ASSET ===\n");
    printf("Investment amount: $%.2f\n\n", ASSET_PURCHASE_AMOUNT);
    printf("1. Cryptocurrency ($%.2f/unit)\n", marketPrices.crypto);
    printf("2. Gold           ($%.2f/unit)\n", marketPrices.gold);
    printf("3. Silver         ($%.2f/unit)\n", marketPrices.silver);
    
    int choice;
    if (!getIntInput("\nChoice: ", &choice)) {
        displayError(ERROR_INVALID_INPUT);
        return;
    }
    
    user->balance -= ASSET_PURCHASE_AMOUNT;
    float units = ASSET_PURCHASE_AMOUNT;
    
    switch (choice) {
        case 1:
            units /= marketPrices.crypto;
            user->assets.crypto += units;
            printf("\n[SUCCESS] Purchased %.4f units of Cryptocurrency\n", units);
            break;
        case 2:
            units /= marketPrices.gold;
            user->assets.gold += units;
            printf("\n[SUCCESS] Purchased %.4f units of Gold\n", units);
            break;
        case 3:
            units /= marketPrices.silver;
            user->assets.silver += units;
            printf("\n[SUCCESS] Purchased %.4f units of Silver\n", units);
            break;
        default:
            user->balance += ASSET_PURCHASE_AMOUNT; // Refund
            displayError(ERROR_INVALID_INPUT);
            return;
    }
    
    printf("Remaining balance: $%.2f\n", user->balance);
    saveAccounts();
}

/**
 * Manage loan (take or repay)
 */
void manageLoan(void) {
    Account *user = &accounts[currentUserIndex];
    
    if (!verifyPIN()) {
        displayError(ERROR_INVALID_PIN);
        return;
    }
    
    printf("\n=== LOAN MANAGEMENT ===\n");
    
    if (user->loan == 0) {
        printf("You have no outstanding loan.\n");
        printf("Would you like to take a loan of $%.2f? (1=Yes, 0=No): ", LOAN_AMOUNT);
        
        int confirm;
        if (!getIntInput("", &confirm) || confirm != 1) {
            printf("Loan request cancelled.\n");
            return;
        }
        
        user->loan = LOAN_AMOUNT;
        user->balance += LOAN_AMOUNT;
        printf("\n[SUCCESS] Loan of $%.2f approved!\n", LOAN_AMOUNT);
        printf("New balance: $%.2f\n", user->balance);
    } else {
        printf("Outstanding loan: $%.2f\n", user->loan);
        printf("Current balance: $%.2f\n", user->balance);
        
        if (user->balance >= user->loan) {
            printf("Repay full loan? (1=Yes, 0=No): ");
            
            int confirm;
            if (!getIntInput("", &confirm) || confirm != 1) {
                printf("Repayment cancelled.\n");
                return;
            }
            
            user->balance -= user->loan;
            user->loan = 0;
            printf("\n[SUCCESS] Loan fully repaid!\n");
            printf("Remaining balance: $%.2f\n", user->balance);
        } else {
            printf("\n[INFO] Insufficient funds to repay loan.\n");
            return;
        }
    }
    
    saveAccounts();
}

/**
 * Add interest to account balance
 */
void addInterest(void) {
    Account *user = &accounts[currentUserIndex];
    float interest = user->balance * INTEREST_RATE;
    
    user->balance += interest;
    
    printf("\n=== INTEREST PAYMENT ===\n");
    printf("Interest rate: %.1f%%\n", INTEREST_RATE * 100);
    printf("Interest earned: $%.2f\n", interest);
    printf("New balance: $%.2f\n", user->balance);
    
    saveAccounts();
}

/**
 * Display comprehensive account status
 */
void displayAccountStatus(void) {
    Account *user = &accounts[currentUserIndex];
    
    // Calculate asset values
    float cryptoValue = user->assets.crypto * marketPrices.crypto;
    float goldValue = user->assets.gold * marketPrices.gold;
    float silverValue = user->assets.silver * marketPrices.silver;
    float totalAssets = cryptoValue + goldValue + silverValue;
    
    // Calculate forex values
    float eurValue = user->currencies.eur * exchangeRates.eur;
    float gbpValue = user->currencies.gbp * exchangeRates.gbp;
    float inrValue = user->currencies.inr * exchangeRates.inr;
    float totalForex = eurValue + gbpValue + inrValue;
    
    // Calculate net worth
    float netWorth = user->balance + totalAssets + totalForex - user->loan;
    
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║        ACCOUNT STATUS REPORT           ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║ Account Holder: %-22s ║\n", user->name);
    printf("╠════════════════════════════════════════╣\n");
    printf("║ CASH                                   ║\n");
    printf("║   Balance:           $%15.2f  ║\n", user->balance);
    printf("║   Loan:             -$%15.2f  ║\n", user->loan);
    printf("╠════════════════════════════════════════╣\n");
    printf("║ ASSETS                                 ║\n");
    printf("║   Crypto: %8.4f units  $%11.2f  ║\n", user->assets.crypto, cryptoValue);
    printf("║   Gold:   %8.4f units  $%11.2f  ║\n", user->assets.gold, goldValue);
    printf("║   Silver: %8.4f units  $%11.2f  ║\n", user->assets.silver, silverValue);
    printf("║   Total Assets:         $%11.2f  ║\n", totalAssets);
    printf("╠════════════════════════════════════════╣\n");
    printf("║ FOREIGN EXCHANGE                       ║\n");
    printf("║   EUR: %10.2f units  $%11.2f  ║\n", user->currencies.eur, eurValue);
    printf("║   GBP: %10.2f units  $%11.2f  ║\n", user->currencies.gbp, gbpValue);
    printf("║   INR: %10.2f units  $%11.2f  ║\n", user->currencies.inr, inrValue);
    printf("║   Total Forex:          $%11.2f  ║\n", totalForex);
    printf("╠════════════════════════════════════════╣\n");
    printf("║ NET WORTH:              $%11.2f  ║\n", netWorth);
    printf("╚════════════════════════════════════════╝\n");
}

/**
 * Manage foreign currency wallet
 */
void manageForexWallet(void) {
    Account *user = &accounts[currentUserIndex];
    
    printf("\n=== FOREX WALLET ===\n");
    printf("USD Balance: $%.2f\n\n", user->balance);
    printf("EUR: %.2f (≈ $%.2f)\n", user->currencies.eur, user->currencies.eur * exchangeRates.eur);
    printf("GBP: %.2f (≈ $%.2f)\n", user->currencies.gbp, user->currencies.gbp * exchangeRates.gbp);
    printf("INR: %.2f (≈ $%.2f)\n\n", user->currencies.inr, user->currencies.inr * exchangeRates.inr);
    
    printf("1. Convert USD → EUR\n");
    printf("2. Convert USD → GBP\n");
    printf("3. Convert USD → INR\n");
    printf("4. Convert Foreign Currency → USD\n");
    printf("5. Back\n");
    
    int choice;
    if (!getIntInput("\nChoice: ", &choice)) {
        displayError(ERROR_INVALID_INPUT);
        return;
    }
    
    if (choice >= 1 && choice <= 3) {
        float amount;
        if (!getFloatInput("Enter USD amount to convert: $", &amount)) {
            displayError(ERROR_INVALID_INPUT);
            return;
        }
        
        if (amount > user->balance) {
            displayError(ERROR_INSUFFICIENT_FUNDS);
            return;
        }
        
        user->balance -= amount;
        
        switch (choice) {
            case 1:
                user->currencies.eur += amount / exchangeRates.eur;
                printf("\n[SUCCESS] Converted $%.2f to %.2f EUR\n", amount, amount / exchangeRates.eur);
                break;
            case 2:
                user->currencies.gbp += amount / exchangeRates.gbp;
                printf("\n[SUCCESS] Converted $%.2f to %.2f GBP\n", amount, amount / exchangeRates.gbp);
                break;
            case 3:
                user->currencies.inr += amount / exchangeRates.inr;
                printf("\n[SUCCESS] Converted $%.2f to %.2f INR\n", amount, amount / exchangeRates.inr);
                break;
        }
        
        saveAccounts();
    } else if (choice == 4) {
        printf("\n1. EUR → USD\n");
        printf("2. GBP → USD\n");
        printf("3. INR → USD\n");
        
        int currencyChoice;
        float amount;
        
        if (!getIntInput("Choice: ", &currencyChoice)) {
            displayError(ERROR_INVALID_INPUT);
            return;
        }
        
        if (!getFloatInput("Enter amount to convert: ", &amount)) {
            displayError(ERROR_INVALID_INPUT);
            return;
        }
        
        bool success = false;
        float usdAmount = 0;
        
        switch (currencyChoice) {
            case 1:
                if (amount <= user->currencies.eur) {
                    user->currencies.eur -= amount;
                    usdAmount = amount * exchangeRates.eur;
                    user->balance += usdAmount;
                    printf("\n[SUCCESS] Converted %.2f EUR to $%.2f\n", amount, usdAmount);
                    success = true;
                }
                break;
            case 2:
                if (amount <= user->currencies.gbp) {
                    user->currencies.gbp -= amount;
                    usdAmount = amount * exchangeRates.gbp;
                    user->balance += usdAmount;
                    printf("\n[SUCCESS] Converted %.2f GBP to $%.2f\n", amount, usdAmount);
                    success = true;
                }
                break;
            case 3:
                if (amount <= user->currencies.inr) {
                    user->currencies.inr -= amount;
                    usdAmount = amount * exchangeRates.inr;
                    user->balance += usdAmount;
                    printf("\n[SUCCESS] Converted %.2f INR to $%.2f\n", amount, usdAmount);
                    success = true;
                }
                break;
        }
        
        if (success) {
            saveAccounts();
        } else {
            displayError(ERROR_INSUFFICIENT_FUNDS);
        }
    }
}

// ==================== MENU SYSTEMS ====================

/**
 * Display main menu (pre-login)
 */
void displayMainMenu(void) {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║    PROFESSIONAL BANKING SYSTEM         ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║  1. Create Account                     ║\n");
    printf("║  2. Login                              ║\n");
    printf("║  3. Exit                               ║\n");
    printf("╚════════════════════════════════════════╝\n");
}

/**
 * Display user menu (post-login)
 */
void displayUserMenu(void) {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║          ACCOUNT OPERATIONS            ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║  1. Cash Transaction (Deposit/Withdraw)║\n");
    printf("║  2. Purchase Assets                    ║\n");
    printf("║  3. Loan Management                    ║\n");
    printf("║  4. Account Status                     ║\n");
    printf("║  5. View Market Prices                 ║\n");
    printf("║  6. Update Market                      ║\n");
    printf("║  7. Add Interest                       ║\n");
    printf("║  8. Forex Wallet                       ║\n");
    printf("║  9. Logout                             ║\n");
    printf("╚════════════════════════════════════════╝\n");
}

// ==================== MAIN PROGRAM ====================

int main(void) {
    // Initialize system
    srand((unsigned int)time(NULL));
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║    PROFESSIONAL BANKING SYSTEM v2.0    ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    // Load existing accounts
    ErrorCode loadResult = loadAccounts();
    if (loadResult == ERROR_FILE_IO) {
        printf("\n[WARNING] Failed to load account data.\n");
    } else {
        printf("\n[INFO] Loaded %d existing account(s).\n", accountCount);
    }
    
    // Main menu loop (pre-login)
    int choice;
    while (true) {
        displayMainMenu();
        
        if (!getIntInput("Choice: ", &choice)) {
            displayError(ERROR_INVALID_INPUT);
            continue;
        }
        
        switch (choice) {
            case 1:
                createAccount();
                break;
            case 2:
                if (loginAccount() == SUCCESS) {
                    goto user_menu; // Break out of main menu into user menu
                }
                break;
            case 3:
                printf("\n[INFO] Thank you for using our banking system. Goodbye!\n");
                return EXIT_SUCCESS;
            default:
                displayError(ERROR_INVALID_INPUT);
        }
    }
    
    // User menu loop (post-login)
user_menu:
    while (true) {
        displayUserMenu();
        
        if (!getIntInput("Choice: ", &choice)) {
            displayError(ERROR_INVALID_INPUT);
            continue;
        }
        
        switch (choice) {
            case 1:
                processCashTransaction();
                break;
            case 2:
                purchaseAsset();
                break;
            case 3:
                manageLoan();
                break;
            case 4:
                displayAccountStatus();
                break;
            case 5:
                displayMarketPrices();
                break;
            case 6:
                updateMarketPrices();
                break;
            case 7:
                addInterest();
                break;
            case 8:
                manageForexWallet();
                break;
            case 9:
                printf("\n[INFO] Logging out... Goodbye, %s!\n", accounts[currentUserIndex].name);
                currentUserIndex = -1;
                return EXIT_SUCCESS;
            default:
                displayError(ERROR_INVALID_INPUT);
        }
    }
    
    return EXIT_SUCCESS;
}
