#include "CryConfigCreator.h"
#include "CryCipher.h"
#include <gitversion/gitversion.h>

using cpputils::Console;
using cpputils::unique_ref;
using cpputils::RandomGenerator;
using std::string;
using std::shared_ptr;
using std::vector;
using boost::optional;
using boost::none;

namespace cryfs {

    CryConfigCreator::CryConfigCreator(shared_ptr<Console> console, RandomGenerator &encryptionKeyGenerator, bool noninteractive)
        :_console(console), _configConsole(console, noninteractive), _encryptionKeyGenerator(encryptionKeyGenerator) {
    }

    CryConfig CryConfigCreator::create(const optional<string> &cipherFromCommandLine) {
        CryConfig config;
        config.SetCipher(_generateCipher(cipherFromCommandLine));
        config.SetVersion(gitversion::VersionString());
        config.SetBlocksizeBytes(_generateBlocksizeBytes());
        config.SetRootBlob(_generateRootBlobKey());
        config.SetEncryptionKey(_generateEncKey(config.Cipher()));
        return config;
    }

    uint32_t CryConfigCreator::_generateBlocksizeBytes() {
        return _configConsole.askBlocksizeBytes();
    }

    string CryConfigCreator::_generateCipher(const optional<string> &cipherFromCommandLine) {
        if (cipherFromCommandLine != none) {
            ASSERT(std::find(CryCiphers::supportedCipherNames().begin(), CryCiphers::supportedCipherNames().end(), *cipherFromCommandLine) != CryCiphers::supportedCipherNames().end(), "Invalid cipher");
            return *cipherFromCommandLine;
        } else {
            return _configConsole.askCipher();
        }
    }

    string CryConfigCreator::_generateEncKey(const std::string &cipher) {
        _console->print("\nGenerating secure encryption key. This might take some time..");
        auto key = CryCiphers::find(cipher).createKey(_encryptionKeyGenerator);
        _console->print("done\n");
        return key;
    }

    string CryConfigCreator::_generateRootBlobKey() {
        //An empty root blob entry will tell CryDevice to create a new root blob
        return "";
    }
}
