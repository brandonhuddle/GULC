#include "OperatorDecl.hpp"

std::string gulc::operatorTypeName(gulc::OperatorType operatorType) {
    switch (operatorType) {
        case OperatorType::Prefix:
            return "prefix";
        case OperatorType::Infix:
            return "infix";
        case OperatorType::Postfix:
            return "postfix";
        default:
            return "unknown";
    }
}
