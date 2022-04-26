#include <set>

#include "cfgparser.h"
#include "secnames.h"
#include "sec_bridge.h"
#include "sec_partial.h"

using namespace e3;
using namespace e3::cr;
using e3::cr::ol::replaceAll;
using std::shared_ptr;

Modular::Modular(std::istream & is, string nm,
                 const std::map<string, string> & globs)
    : SecType(nm), beta(0)
{
    std::map<string, string *> kv = stdParams();

    kv[secNames::postfix] = &postfixP;
    kv[secNames::postneg] = &postfixN;
    kv[secNames::polyModulusDegree1] = &polyModulusDegree;
    kv[secNames::plaintextModulus1] = &plaintextModulus;
    kv[secNames::polyModulusDegree2] = &polyModulusDegree;
    kv[secNames::plaintextModulus2] = &plaintextModulus;
    kv[secNames::encoder] = &encoder;
    kv[secNames::scale] = &scale;
    kv[secNames::primes] = &primes;

    string slambda, sbeta;
    kv[secNames::lambda] = &slambda;
    kv[secNames::pailBeta] = &sbeta;
    kv[secNames::copheeIsArduino] = &copheeIsArduino;
    kv[secNames::copheeBaudRate] = &copheeBaudRate;

    loadPairs(is, kv);
    globPairs(kv, globs);

    fixEncType();

    if ( sbeta.empty() ) beta = 0;
    else beta = std::stoi(sbeta);

    std::vector<string> valid_names {secNames::encPila, secNames::encPila,
                                     secNames::encPail, secNames::encPailg,
                                     secNames::encSeal, secNames::encSealCkks,
                                     secNames::encBfvProt
                                    };

    if ( encType.empty() ) throw "encryption type must be defined for " + nm;
    else if ( e3::cr::ol::isin(valid_names, encType) ) {}

    else if ( encType[0] == '@' ) {}

    else
    {
        string s; for ( auto x : valid_names ) s += ' ' + x;
        throw "(Modular) Bad encryption type [" + encType + "]; valid:" + s + " )";
    }

    if ( encType[0] != '@' )
    {
        if ( slambda.empty() ) lambda = 2;
        else lambda = std::stoi(slambda);
    }
}

void Modular::genKeys(bool forceGen, bool forceLoad,
                      std::string seed, const ConfigParser * par)
{
    seed = name.typ + seed;

    if (0) {}

    else if ( encType == secNames::encPila )
        sk = shared_ptr<PrivKey>
             (new PilaPrivKey(name, forceGen, forceLoad, seed, lambda));

    else if ( encType == secNames::encPail )
        sk = shared_ptr<PrivKey>
             (new PailPrivKey(name, forceGen, forceLoad, seed, lambda));

    else if ( encType == secNames::encPailg )
        sk = shared_ptr<PrivKey>
             (new PailgPrivKey(name, forceGen, forceLoad, seed, lambda, beta));

    else if ( encType == secNames::encSeal )
        sk = shared_ptr<PrivKey>
             (new SealPrivKey
              (name, forceGen, forceLoad, seed,
               lambda, polyModulusDegree, plaintextModulus, encoder));

    else if ( encType == secNames::encBfvProt )
        sk = shared_ptr<PrivKey>
             (new BfvProtPrivKey
              (name, forceGen, forceLoad, seed,
               lambda, polyModulusDegree, plaintextModulus, encoder));

    else if ( encType == secNames::encSealCkks )
        sk = shared_ptr<PrivKey>
             (new SealCkksPrivKey
              (name, forceGen, forceLoad, seed,
               lambda, polyModulusDegree, primes, scale));

    else if (encType[0] == '@')
    {
        makeBridge(par, 1); if (!bridge) never("bridge");

        if (0) {}

        else if (encType == secNames::encPila)
        {
            PilBasePrivKey * p = dynamic_cast<PilBasePrivKey *>(bridge->get_sk_raw());
            if (!p) never("bad bridge");
            sk = shared_ptr<PrivKey>(new PilaPrivKey(*p, name.typ));
        }

        else if (encType == secNames::encSeal)
        {
            SealBasePrivKey * p = dynamic_cast<SealBasePrivKey *>(bridge->get_sk_raw());
            if (!p) never("bad bridge");
            sk = shared_ptr<PrivKey>(new SealPrivKey(*p, name.typ));
        }

        else
            throw "Circuit: Bridge is not supperted for type ["
            + encType + "] in " + name.typ;
    }

    else
        throw "(Modular::genKeys) Bad encryption type [" + encType + "] in " + name.typ;

    // set max size for constants
    if ( plaintext_size < 0 ) plaintext_size = lambda - 1;
}

void Modular::writeH(string root, std::ostream & os, string user_dir) const
{
    string dbf = cfgNames::dotH(cfgNames::dbfileModular + '.' + encType);

    using namespace secNames;

    string f = loadDbTemplAri(root, dbf);

    auto allconsts = find_constants(user_dir);
    ol::replaceAll(f, R_postfixDefines, makeDefines(allconsts) );

    os << f;
}

void Modular::writeInc(string root, std::ostream & os) const
{
    string dbf = cfgNames::dotInc(cfgNames::dbfileModular + '.' + encType + implVer());

    string f = loadDbTemplAri(root, dbf);
    os << f;
}

void Modular::writeCpp(string root, std::ostream & os) const
{
    string dbf = cfgNames::dotCpp(cfgNames::dbfileModular + '.' + encType + implVer());

    string f = loadDbTemplAri(root, dbf);
    os << f;
}

void Modular::fixEncType()
{
    if ( encType == secNames::encPilBase ) { encType = secNames::encPila; }
}

string Modular::loadDbTemplAri(string root, string fn) const
{
    using namespace secNames;

    string f = loadDbTemplRep(root, fn);

    if ( f.find(R_ariZero) != string::npos )
    {
        string n = "0";
        for ( size_t i = 1; i < sk->slots(); i++ ) n += "_0";
        ol::replaceAll(f, R_ariZero, longConstTyp(sk->encrypt(n, 1)));
    }

    if ( f.find(R_ariUnit) != string::npos )
    {
        string n = "1";
        for ( size_t i = 1; i < sk->slots(); i++ ) n += "_1";
        ol::replaceAll(f, R_ariUnit, longConstTyp(sk->encrypt(n, 1)));
    }

    {
        // fkf of PailG
        auto p = dynamic_cast<PailgPrivKey *>(sk.get());
        if ( p ) ol::replaceAll(f, R_Pailgfkf, p->getFkf().str());
        ol::replaceAll(f, R_copheeIsArduino, copheeIsArduino);
        ol::replaceAll(f, R_copheeBaudRate, copheeBaudRate);
    }

    return f;
}
