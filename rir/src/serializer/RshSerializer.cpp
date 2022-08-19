#include "RshSerializer.h"
#include "R/Protect.h"
#include <fstream>
#include <sstream>
#include "serializer/serializerData.h"

#include "serializerDeserializerGeneral/DebugMessages.h"

namespace rir {

static inline bool fileExists(std::string fName) {
    std::ifstream f(fName.c_str());
    return f.good();
}

void RshSerializer::serializeFunction(SEXP hast, const unsigned & indexOffset, const std::string & name, SEXP cData, bool & serializerError) {
    Protect protecc;
    DebugMessages::printSerializerMessage("(*) serializeClosure start", 1);
    auto prefix = getenv("PIR_SERIALIZE_PREFIX") ? getenv("PIR_SERIALIZE_PREFIX") : "bitcodes";
    std::stringstream fN;
    fN << prefix << "/" << "m_" << CHAR(PRINTNAME(hast)) << ".meta";
    std::string fName = fN.str();

    SEXP sDataContainer;

    if (fileExists(fName)) {
        DebugMessages::printSerializerMessage("(*) metadata already exists", 2);

        FILE *reader;
        reader = fopen(fName.c_str(),"r");

        if (!reader) {
            serializerError = true;
            DebugMessages::printSerializerMessage("(*) serializeClosure failed, unable to open existing metadata", 1);
            return;
        }

        R_inpstream_st inputStream;
        R_InitFileInPStream(&inputStream, reader, R_pstream_binary_format, NULL, R_NilValue);

        SEXP result;
        protecc(result= R_Unserialize(&inputStream));

        sDataContainer = result;

        //
        // Correct captured name if this is the parent closure i.e. offset == 0
        //

        if (indexOffset == 0) {
            serializerData::addName(sDataContainer, Rf_install(name.c_str()));
        }

        fclose(reader);
    } else {
        protecc(sDataContainer = Rf_allocVector(VECSXP, serializerData::getStorageSize()));

        serializerData::addHast(sDataContainer, hast);

        //
        // Temporarily store last stored inner functions name, until parent is added
        //

        std::stringstream nameStr;
        nameStr << "Inner(" << indexOffset << "): " << name << std::endl;

        serializerData::addName(sDataContainer, Rf_install(nameStr.str().c_str()));
    }
    // Add context data
    std::string offsetStr(std::to_string(indexOffset));
    SEXP offsetSym = Rf_install(offsetStr.c_str());

    std::string conStr(std::to_string(contextData::getContext(cData)));
    SEXP contextSym = Rf_install(conStr.c_str());

    SEXP epoch = serializerData::addBitcodeData(sDataContainer, offsetSym, contextSym, cData);

    // 2. Write updated metadata
    R_outpstream_st outputStream;
    FILE *fptr;
    fptr = fopen(fName.c_str(),"w");
    if (!fptr) {
        serializerError = true;
        DebugMessages::printSerializerMessage("(*) serializeClosure failed, unable to write metadata", 1);
        return;
    }
    R_InitFileOutPStream(&outputStream,fptr,R_pstream_binary_format, 0, NULL, R_NilValue);
    R_Serialize(sDataContainer, &outputStream);
    fclose(fptr);

    if (DebugMessages::serializerDebugLevel() > 1) {
        serializerData::print(sDataContainer, 2);
    }

    // rename temp files
    {
        std::stringstream bcFName;
        std::stringstream bcOldName;
        bcFName << prefix << "/" << CHAR(PRINTNAME(hast)) << "_" << indexOffset << "_" << CHAR(PRINTNAME(epoch)) << ".bc";
        bcOldName << prefix << "/" << contextData::getContext(cData) << ".bc";
        int stat = std::rename(bcOldName.str().c_str(), bcFName.str().c_str());
        if (stat != 0) {
            serializerError = true;
            DebugMessages::printSerializerMessage("(*) serializeClosure failed, unable to rename bitcode.", 1);
            return;
        }
    }

    {
        std::stringstream bcFName;
        std::stringstream bcOldName;
        bcFName << prefix << "/" << CHAR(PRINTNAME(hast)) << "_" << indexOffset << "_" << CHAR(PRINTNAME(epoch)) << ".pool";
        bcOldName << prefix << "/" << contextData::getContext(cData) << ".pool";
        int stat = std::rename(bcOldName.str().c_str(), bcFName.str().c_str());
        if (stat != 0) {
            serializerError = true;
            DebugMessages::printSerializerMessage("(*) serializeClosure failed, unable to rename pool.", 1);
            return;
        }
    }

}

void RshSerializer::populateTypeFeedbackData(SEXP container, DispatchTable * vtab) {
    DispatchTable * currVtab = vtab;

    std::function<void(Code *, Function *)> iterateOverCodeObjs = [&] (Code * c, Function * funn) {
        // Default args
        if (funn) {
            auto nargs = funn->nargs();
            for (unsigned i = 0; i < nargs; i++) {
                auto code = funn->defaultArg(i);
                if (code != nullptr) {
                    iterateOverCodeObjs(code, nullptr);
                }
            }
        }

        Opcode* pc = c->code();
        std::vector<BC::FunIdx> promises;
        Protect p;
        while (pc < c->endCode()) {
            BC bc = BC::decode(pc, c);
            bc.addMyPromArgsTo(promises);

            // call sites
            switch (bc.bc) {
                case Opcode::record_type_:

                    // std::cout << "record_type_(" << i << "): ";
                    // bc.immediate.typeFeedback.print(std::cout);
                    // std::cout << std::endl;
                    // std::cout << "[[ " << &bc.immediate.typeFeedback << " ]]" << std::endl;
                    contextData::addObservedValueToVector(container, &bc.immediate.typeFeedback);

                default: {}
            }

            // inner functions
            if (bc.bc == Opcode::push_ && TYPEOF(bc.immediateConst()) == EXTERNALSXP) {
                SEXP iConst = bc.immediateConst();
                if (DispatchTable::check(iConst)) {
                    currVtab = DispatchTable::unpack(iConst);
                    auto c = currVtab->baseline()->body();
                    auto f = c->function();
                    iterateOverCodeObjs(c, f);
                }
            }

            pc = BC::next(pc);
        }

        // Iterate over promises code objects recursively
        for (auto i : promises) {
            auto prom = c->getPromise(i);
            iterateOverCodeObjs(prom, nullptr);
        }
    };

    Code * genesisCodeObj = currVtab->baseline()->body();
    Function * genesisFunObj = genesisCodeObj->function();

    iterateOverCodeObjs(genesisCodeObj, genesisFunObj);
}


} // namespace rir
