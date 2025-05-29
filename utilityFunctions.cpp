#include "utilityFunctions.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

using namespace std;
using namespace rapidjson;



string convertToHexString(const unsigned char* data, int dataSize) {
    ostringstream oss;
    oss << hex << setfill('0');
    for (int i = 0; i < dataSize; ++i) {
        oss << setw(2) << static_cast<unsigned int>(data[i]);
    }
    return oss.str();
}
string sha256(const string& source) {
    const EVP_MD* md = EVP_get_digestbyname("SHA256");
    if (!md) {
        cerr << "Unknown message digest SHA256" << endl;
        return "";
    }

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        cerr << "Failed to create EVP_MD_CTX" << endl;
        return "";
    }

    if (EVP_DigestInit_ex(mdctx, md, nullptr) != 1) {
        cerr << "EVP_DigestInit_ex failed" << endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    if (EVP_DigestUpdate(mdctx, source.data(), source.size()) != 1) {
        cerr << "EVP_DigestUpdate failed" << endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;

    if (EVP_DigestFinal_ex(mdctx, md_value, &md_len) != 1) {
        cerr << "EVP_DigestFinal_ex failed" << endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    EVP_MD_CTX_free(mdctx);

    return convertToHexString(md_value, md_len);
}


UserType authenticate(string dataFile)
{
    ifstream ifs(dataFile);
    if (!ifs.is_open())
    {
        cerr << "Error: Cannot open user data file: " << dataFile << endl;
        return invalid;
    }

    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);

    if (!doc.IsObject() || !doc.HasMember("Users") || !doc["Users"].IsArray())
    {
        cerr << "Error: userData.json should contain a 'Users' array.\n";
        return invalid;
    }

    const Value &usersArray = doc["Users"];

    string username, password;
    cout << "Login Name: ";
    cin >> username;
    cout << "Password: ";
    cin >> password;

    string hashedInput = sha256(password);

    for (SizeType i = 0; i < usersArray.Size(); ++i)
    {
        const Value &userObj = usersArray[i];

        if (userObj.HasMember("loginname") && userObj["loginname"].IsString() &&
            userObj.HasMember("pwSHA256ChkSum") && userObj["pwSHA256ChkSum"].IsString() &&
            userObj.HasMember("role") && userObj["role"].IsString())
        {

            string storedUsername = userObj["loginname"].GetString();
            string storedHash = userObj["pwSHA256ChkSum"].GetString();
            string storedRole = userObj["role"].GetString();
            if (storedUsername == username && storedHash == hashedInput)
            {
                cout << "Welcom to Fleet Xpress:" << username << endl;

                if (storedRole == "admin")
                    return admin;
                if (storedRole == "user")
                    return user;

                return invalid; // unknown role
            }
        }
    }

    return invalid; // no match
}
