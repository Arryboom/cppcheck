/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2014 Daniel Marjamäki and Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


//---------------------------------------------------------------------------
#include "checknullpointer.h"
#include "mathlib.h"
#include "symboldatabase.h"
#include <cctype>
//---------------------------------------------------------------------------

// Register this check class (by creating a static instance of it)
namespace {
    CheckNullPointer instance;
}

//---------------------------------------------------------------------------

/**
 * @brief parse a function call and extract information about variable usage
 * @param tok first token
 * @param var variables that the function read / write.
 * @param library --library files data
 * @param value 0 => invalid with null pointers as parameter.
 *              1-.. => only invalid with uninitialized data.
 */
void CheckNullPointer::parseFunctionCall(const Token &tok, std::list<const Token *> &var, const Library *library, unsigned char value)
{
    // standard functions that dereference first parameter..
    static std::set<std::string> functionNames1_all;     // used no matter what 'value' is
    static std::set<std::string> functionNames1_nullptr; // used only when 'value' is 0
    static std::set<std::string> functionNames1_uninit;  // used only when 'value' is non-zero
    if (functionNames1_all.empty()) {
        // cstdlib
        functionNames1_all.insert("atoi");
        functionNames1_all.insert("atof");
        functionNames1_all.insert("atol");
        functionNames1_all.insert("qsort");
        functionNames1_all.insert("strtof");
        functionNames1_all.insert("strtod");
        functionNames1_all.insert("strtol");
        functionNames1_all.insert("strtoul");
        functionNames1_all.insert("strtold");
        functionNames1_all.insert("strtoll");
        functionNames1_all.insert("strtoull");
        functionNames1_all.insert("wcstof");
        functionNames1_all.insert("wcstod");
        functionNames1_all.insert("wcstol");
        functionNames1_all.insert("wcstoul");
        functionNames1_all.insert("wcstold");
        functionNames1_all.insert("wcstoll");
        functionNames1_all.insert("wcstoull");
        // cstring
        functionNames1_all.insert("strcat");
        functionNames1_all.insert("strncat");
        functionNames1_all.insert("strcoll");
        functionNames1_all.insert("strchr");
        functionNames1_all.insert("strrchr");
        functionNames1_all.insert("strcmp");
        functionNames1_all.insert("strncmp");
        functionNames1_all.insert("strcspn");
        functionNames1_all.insert("strdup");
        functionNames1_all.insert("strndup");
        functionNames1_all.insert("strpbrk");
        functionNames1_all.insert("strlen");
        functionNames1_all.insert("strspn");
        functionNames1_all.insert("strstr");
        functionNames1_all.insert("wcscat");
        functionNames1_all.insert("wcsncat");
        functionNames1_all.insert("wcscoll");
        functionNames1_all.insert("wcschr");
        functionNames1_all.insert("wcsrchr");
        functionNames1_all.insert("wcscmp");
        functionNames1_all.insert("wcsncmp");
        functionNames1_all.insert("wcscspn");
        functionNames1_all.insert("wcsdup");
        functionNames1_all.insert("wcsndup");
        functionNames1_all.insert("wcspbrk");
        functionNames1_all.insert("wcslen");
        functionNames1_all.insert("wcsspn");
        functionNames1_all.insert("wcsstr");
        // cstdio
        functionNames1_all.insert("fclose");
        functionNames1_all.insert("feof");
        functionNames1_all.insert("fwrite");
        functionNames1_all.insert("fseek");
        functionNames1_all.insert("ftell");
        functionNames1_all.insert("fputs");
        functionNames1_all.insert("fputws");
        functionNames1_all.insert("ferror");
        functionNames1_all.insert("fgetc");
        functionNames1_all.insert("fgetwc");
        functionNames1_all.insert("fgetpos");
        functionNames1_all.insert("fsetpos");
        functionNames1_all.insert("fscanf");
        functionNames1_all.insert("fprintf");
        functionNames1_all.insert("fwscanf");
        functionNames1_all.insert("fwprintf");
        functionNames1_all.insert("fopen");
        functionNames1_all.insert("rewind");
        functionNames1_all.insert("printf");
        functionNames1_all.insert("wprintf");
        functionNames1_all.insert("scanf");
        functionNames1_all.insert("wscanf");
        functionNames1_all.insert("fscanf");
        functionNames1_all.insert("sscanf");
        functionNames1_all.insert("fwscanf");
        functionNames1_all.insert("swscanf");
        functionNames1_all.insert("setbuf");
        functionNames1_all.insert("setvbuf");
        functionNames1_all.insert("rename");
        functionNames1_all.insert("remove");
        functionNames1_all.insert("puts");
        functionNames1_all.insert("getc");
        functionNames1_all.insert("clearerr");
        // ctime
        functionNames1_all.insert("asctime");
        functionNames1_all.insert("ctime");
        functionNames1_all.insert("mktime");

        functionNames1_nullptr.insert("strcpy");
        functionNames1_nullptr.insert("sprintf");
        functionNames1_nullptr.insert("vsprintf");
        functionNames1_nullptr.insert("vprintf");
        functionNames1_nullptr.insert("fprintf");
        functionNames1_nullptr.insert("vfprintf");
        functionNames1_nullptr.insert("wcscpy");
        functionNames1_nullptr.insert("swprintf");
        functionNames1_nullptr.insert("vswprintf");
        functionNames1_nullptr.insert("vwprintf");
        functionNames1_nullptr.insert("fwprintf");
        functionNames1_nullptr.insert("vfwprintf");
        functionNames1_nullptr.insert("fread");
        functionNames1_nullptr.insert("gets");
        functionNames1_nullptr.insert("gmtime");
        functionNames1_nullptr.insert("localtime");
        functionNames1_nullptr.insert("strftime");

        functionNames1_uninit.insert("itoa"); // value to convert
        functionNames1_uninit.insert("perror");
        functionNames1_uninit.insert("fflush");
        functionNames1_uninit.insert("freopen");
    }

    // standard functions that dereference second parameter..
    static std::set<std::string> functionNames2_all;     // used no matter what 'value' is
    static std::set<std::string> functionNames2_nullptr; // used only if 'value' is 0
    if (functionNames2_all.empty()) {
        functionNames2_all.insert("mbstowcs");
        functionNames2_all.insert("wcstombs");
        functionNames2_all.insert("strcat");
        functionNames2_all.insert("strncat");
        functionNames2_all.insert("strcmp");
        functionNames2_all.insert("strncmp");
        functionNames2_all.insert("strcoll");
        functionNames2_all.insert("strcpy");
        functionNames2_all.insert("strcspn");
        functionNames2_all.insert("strncpy");
        functionNames2_all.insert("strpbrk");
        functionNames2_all.insert("strspn");
        functionNames2_all.insert("strstr");
        functionNames2_all.insert("strxfrm");
        functionNames2_all.insert("wcscat");
        functionNames2_all.insert("wcsncat");
        functionNames2_all.insert("wcscmp");
        functionNames2_all.insert("wcsncmp");
        functionNames2_all.insert("wcscoll");
        functionNames2_all.insert("wcscpy");
        functionNames2_all.insert("wcscspn");
        functionNames2_all.insert("wcsncpy");
        functionNames2_all.insert("wcspbrk");
        functionNames2_all.insert("wcsspn");
        functionNames2_all.insert("wcsstr");
        functionNames2_all.insert("wcsxfrm");
        functionNames2_all.insert("sprintf");
        functionNames2_all.insert("fprintf");
        functionNames2_all.insert("fscanf");
        functionNames2_all.insert("sscanf");
        functionNames2_all.insert("swprintf");
        functionNames2_all.insert("fwprintf");
        functionNames2_all.insert("fwscanf");
        functionNames2_all.insert("swscanf");
        functionNames2_all.insert("fputs");
        functionNames2_all.insert("fputc");
        functionNames2_all.insert("ungetc");
        functionNames2_all.insert("fputws");
        functionNames2_all.insert("fputwc");
        functionNames2_all.insert("ungetwc");
        functionNames2_all.insert("rename");
        functionNames2_all.insert("putc");
        functionNames2_all.insert("putwc");
        functionNames2_all.insert("freopen");

        functionNames2_nullptr.insert("itoa"); // destination buffer
        functionNames2_nullptr.insert("frexp");
        functionNames2_nullptr.insert("modf");
        functionNames2_nullptr.insert("fgetpos");
    }

    if (Token::Match(&tok, "%var% ( )") || !tok.tokAt(2))
        return;

    const Token* firstParam = tok.tokAt(2);
    const Token* secondParam = firstParam->nextArgument();

    // 1st parameter..
    if ((Token::Match(firstParam, "%var% ,|)") && firstParam->varId() > 0) ||
        (value == 0 && Token::Match(firstParam, "0|NULL ,|)"))) {
        if (functionNames1_all.find(tok.str()) != functionNames1_all.end())
            var.push_back(firstParam);
        else if (value == 0 && functionNames1_nullptr.find(tok.str()) != functionNames1_nullptr.end())
            var.push_back(firstParam);
        else if (value != 0 && functionNames1_uninit.find(tok.str()) != functionNames1_uninit.end())
            var.push_back(firstParam);
        else if (value == 0 && Token::Match(&tok, "snprintf|vsnprintf|fnprintf|vfnprintf") && secondParam && secondParam->str() != "0") // Only if length (second parameter) is not zero
            var.push_back(firstParam);
        else if (value == 0 && library != nullptr && library->isnullargbad(tok.str(),1))
            var.push_back(firstParam);
        else if (value == 1 && library != nullptr && library->isuninitargbad(tok.str(),1))
            var.push_back(firstParam);
    }

    // 2nd parameter..
    if ((value == 0 && Token::Match(secondParam, "0|NULL ,|)")) || (secondParam && secondParam->varId() > 0 && Token::Match(secondParam->next(),"[,)]"))) {
        if (functionNames2_all.find(tok.str()) != functionNames2_all.end())
            var.push_back(secondParam);
        else if (value == 0 && functionNames2_nullptr.find(tok.str()) != functionNames2_nullptr.end())
            var.push_back(secondParam);
        else if (value == 0 && library != nullptr && library->isnullargbad(tok.str(),2))
            var.push_back(secondParam);
        else if (value == 1 && library != nullptr && library->isuninitargbad(tok.str(),2))
            var.push_back(secondParam);
    }

    if (Token::Match(&tok, "printf|sprintf|snprintf|fprintf|fnprintf|scanf|sscanf|fscanf|wprintf|swprintf|fwprintf|wscanf|swscanf|fwscanf")) {
        const Token* argListTok = 0; // Points to first va_list argument
        std::string formatString;
        bool scan = Token::Match(&tok, "scanf|sscanf|fscanf|wscanf|swscanf|fwscanf");

        if (Token::Match(&tok, "printf|scanf|wprintf|wscanf ( %str%")) {
            formatString = firstParam->strValue();
            argListTok = secondParam;
        } else if (Token::Match(&tok, "sprintf|fprintf|sscanf|fscanf|fwprintf|fwscanf|swscanf")) {
            const Token* formatStringTok = secondParam; // Find second parameter (format string)
            if (formatStringTok && formatStringTok->type() == Token::eString) {
                argListTok = formatStringTok->nextArgument(); // Find third parameter (first argument of va_args)
                formatString = formatStringTok->strValue();
            }
        } else if (Token::Match(&tok, "snprintf|fnprintf|swprintf") && secondParam) {
            const Token* formatStringTok = secondParam->nextArgument(); // Find third parameter (format string)
            if (formatStringTok && formatStringTok->type() == Token::eString) {
                argListTok = formatStringTok->nextArgument(); // Find fourth parameter (first argument of va_args)
                formatString = formatStringTok->strValue();
            }
        }

        if (argListTok) {
            bool percent = false;
            for (std::string::iterator i = formatString.begin(); i != formatString.end(); ++i) {
                if (*i == '%') {
                    percent = !percent;
                } else if (percent) {
                    percent = false;

                    bool _continue = false;
                    while (!std::isalpha(*i)) {
                        if (*i == '*') {
                            if (scan)
                                _continue = true;
                            else
                                argListTok = argListTok->nextArgument();
                        }
                        ++i;
                        if (!argListTok || i == formatString.end())
                            return;
                    }
                    if (_continue)
                        continue;

                    if ((*i == 'n' || *i == 's' || scan) && (!scan || value == 0)) {
                        if ((value == 0 && argListTok->str() == "0") || (argListTok->varId() > 0 && Token::Match(argListTok,"%var% [,)]"))) {
                            var.push_back(argListTok);
                        }
                    }

                    if (*i != 'm') // %m is a non-standard glibc extension that requires no parameter
                        argListTok = argListTok->nextArgument(); // Find next argument
                    if (!argListTok)
                        break;
                }
            }
        }
    }
}


/**
 * Is there a pointer dereference? Everything that should result in
 * a nullpointer dereference error message will result in a true
 * return value. If it's unknown if the pointer is dereferenced false
 * is returned.
 * @param tok token for the pointer
 * @param unknown it is not known if there is a pointer dereference (could be reported as a debug message)
 * @return true => there is a dereference
 */
bool CheckNullPointer::isPointerDeRef(const Token *tok, bool &unknown)
{
    // THIS ARRAY MUST BE ORDERED ALPHABETICALLY
    static const char* const stl_stream [] = {
        "fstream", "ifstream", "iostream", "istream",
        "istringstream", "ofstream", "ostream", "ostringstream",
        "stringstream", "wistringstream", "wostringstream", "wstringstream"
    };

    const bool inconclusive = unknown;

    unknown = false;

    const Token* prev = tok->previous();
    while (prev->str() == "." || prev->str() == "::") { // Skip over previous member dereferences
        prev = prev->previous();
        while (prev->link() && (prev->str() == "]" || prev->str() == ">"))
            prev = prev->link()->previous();
        prev = prev->previous();
    }
    while (prev->str() == ")") // Skip over casts
        prev = prev->link()->previous();

    // Dereferencing pointer..
    if (prev->str() == "*" && (Token::Match(prev->previous(), "return|throw|;|{|}|:|[|(|,") || prev->previous()->isOp()) && !Token::Match(prev->tokAt(-2), "sizeof|decltype|typeof"))
        return true;

    // read/write member variable
    if (!Token::simpleMatch(prev->previous(), "& (") && !Token::Match(prev->previous(), "sizeof|decltype|typeof (") && prev->str() != "&" && Token::Match(tok->next(), ". %var%")) {
        if (tok->strAt(3) != "(")
            return true;
        unknown = true;
        return false;
    }

    if (Token::Match(tok, "%var% [") && (prev->str() != "&" || Token::Match(tok->next()->link()->next(), "[.(]")))
        return true;

    if (Token::Match(tok, "%var% ("))
        return true;

    if (Token::Match(tok, "%var% = %var% .") &&
        tok->varId() > 0 &&
        tok->varId() == tok->tokAt(2)->varId())
        return true;

    // std::string dereferences nullpointers
    if (Token::Match(prev->tokAt(-3), "std :: string|wstring (") && tok->strAt(1) == ")")
        return true;
    if (Token::Match(prev->previous(), "%var% (") && tok->strAt(1) == ")") {
        const Variable* var = tok->tokAt(-2)->variable();
        if (var && !var->isPointer() && !var->isArray() && Token::Match(var->typeStartToken(), "std :: string|wstring !!::"))
            return true;
    }

    // streams dereference nullpointers
    if (Token::Match(prev, "<<|>>")) {
        const Variable* var = tok->variable();
        if (var && var->isPointer() && Token::Match(var->typeStartToken(), "char|wchar_t")) { // Only outputting or reading to char* can cause problems
            const Token* tok2 = prev; // Find start of statement
            for (; tok2; tok2 = tok2->previous()) {
                if (Token::Match(tok2->previous(), ";|{|}|:"))
                    break;
            }
            if (Token::Match(tok2, "std :: cout|cin|cerr"))
                return true;
            if (tok2 && tok2->varId() != 0) {
                const Variable* var2 = tok2->variable();
                if (var2 && var2->isStlType(stl_stream))
                    return true;
            }
        }
    }

    const Variable *ovar = nullptr;
    if (Token::Match(tok, "%var% ==|!= %var%"))
        ovar = tok->tokAt(2)->variable();
    else if (Token::Match(prev->previous(), "%var% ==|!="))
        ovar = tok->tokAt(-2)->variable();
    else if (Token::Match(prev->previous(), "%var% =|+") && Token::Match(tok->next(), ")|]|,|;|+"))
        ovar = tok->tokAt(-2)->variable();
    if (ovar && !ovar->isPointer() && !ovar->isArray() && Token::Match(ovar->typeStartToken(), "std :: string|wstring !!::"))
        return true;

    // Check if it's NOT a pointer dereference.
    // This is most useful in inconclusive checking
    if (inconclusive) {
        // Not a dereference..
        if (Token::Match(tok, "%var% ="))
            return false;

        // OK to delete a null
        if (Token::simpleMatch(prev, "delete") || Token::Match(prev->tokAt(-2), "delete [ ]"))
            return false;

        // OK to check if pointer is null
        // OK to take address of pointer
        if (Token::Match(prev, "!|&"))
            return false;

        // OK to check pointer in "= p ? : "
        if (tok->next()->str() == "?" &&
            (Token::Match(prev, "return|throw|;|{|}|:|[|(|,") || prev->isAssignmentOp()))
            return false;

        // OK to pass pointer to function
        if (Token::Match(prev, "[(,]") && Token::Match(tok->next(), "[,)]") &&
            (prev->str() != "(" ||
             Token::Match(prev->previous(), "%var% (")))
            return false;

        // Compare pointer
        if (Token::Match(prev, "(|&&|%oror%|==|!="))
            return false;
        if (Token::Match(tok, "%var% &&|%oror%|==|!=|)"))
            return false;

        // Taking address
        if (Token::Match(prev, "return|=") && tok->strAt(1) == ";")
            return false;

        // (void)var
        if (Token::Match(prev, "[{;}]") && tok->strAt(1) == ";")
            return false;

        // Shift pointer (e.g. to cout, but its no char* (see above))
        if (Token::Match(prev, "<<|>>"))
            return false;

        // unknown if it's a dereference
        unknown = true;
    }

    // assume that it's not a dereference (no false positives)
    return false;
}


// check if function can assign pointer
bool CheckNullPointer::CanFunctionAssignPointer(const Token *functiontoken, unsigned int varid, bool& unknown)
{
    if (Token::Match(functiontoken, "if|while|for|switch|sizeof|catch"))
        return false;

    unsigned int argumentNumber = 0;
    for (const Token *arg = functiontoken->tokAt(2); arg; arg = arg->nextArgument()) {
        if (Token::Match(arg, "%varid% [,)]", varid)) {
            const Function* func = functiontoken->function();
            if (!func) { // Unknown function
                unknown = true;
                return true; // assume that the function might assign the pointer
            }

            const Variable* var = func->getArgumentVar(argumentNumber);
            if (!var) { // Unknown variable
                unknown = true;
                return true;
            } else if (var->isReference()) // Assume every pointer passed by reference is assigned
                return true;
            else
                return false;
        }
        ++argumentNumber;
    }

    // pointer is not passed
    return false;
}



void CheckNullPointer::nullPointerLinkedList()
{
    const SymbolDatabase* const symbolDatabase = _tokenizer->getSymbolDatabase();

    // looping through items in a linked list in a inner loop.
    // Here is an example:
    //    for (const Token *tok = tokens; tok; tok = tok->next) {
    //        if (tok->str() == "hello")
    //            tok = tok->next;   // <- tok might become a null pointer!
    //    }
    for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) {
        const Token* const tok1 = i->classDef;
        // search for a "for" scope..
        if (i->type != Scope::eFor || !tok1)
            continue;

        // is there any dereferencing occurring in the for statement
        const Token* end2 = tok1->linkAt(1);
        for (const Token *tok2 = tok1->tokAt(2); tok2 != end2; tok2 = tok2->next()) {
            // Dereferencing a variable inside the "for" parentheses..
            if (Token::Match(tok2, "%var% . %var%")) {
                // Is this variable a pointer?
                const Variable *var = tok2->variable();
                if (!var || !var->isPointer())
                    continue;

                // Variable id for dereferenced variable
                const unsigned int varid(tok2->varId());

                // We don't support variables without a varid
                if (varid == 0)
                    continue;

                if (Token::Match(tok2->tokAt(-2), "%varid% ?", varid))
                    continue;

                // Check usage of dereferenced variable in the loop..
                for (std::list<Scope*>::const_iterator j = i->nestedList.begin(); j != i->nestedList.end(); ++j) {
                    Scope* scope = *j;
                    if (scope->type != Scope::eWhile)
                        continue;

                    // TODO: are there false negatives for "while ( %varid% ||"
                    if (Token::Match(scope->classDef->next(), "( %varid% &&|)", varid)) {
                        // Make sure there is a "break" or "return" inside the loop.
                        // Without the "break" a null pointer could be dereferenced in the
                        // for statement.
                        for (const Token *tok4 = scope->classStart; tok4; tok4 = tok4->next()) {
                            if (tok4 == i->classEnd) {
                                nullPointerError(tok1, var->name(), scope->classDef);
                                break;
                            }

                            // There is a "break" or "return" inside the loop.
                            // TODO: there can be false negatives. There could still be
                            //       execution paths that are not properly terminated
                            else if (tok4->str() == "break" || tok4->str() == "return")
                                break;
                        }
                    }
                }
            }
        }
    }
}

void CheckNullPointer::nullPointerByDeRefAndChec()
{
    for (const Token *tok = _tokenizer->tokens(); tok; tok = tok->next()) {
        const Variable *var = tok->variable();
        if (!var || !var->isPointer() || tok == var->nameToken())
            continue;

        // Can pointer be NULL?
        const ValueFlow::Value *value = tok->getValue(0);
        if (!value)
            continue;

        if (!_settings->inconclusive && value->inconclusive)
            continue;

        // Is pointer used as function parameter?
        if (Token::Match(tok->previous(), "[(,] %var% [,)]")) {
            const Token *ftok = tok->previous();
            while (ftok && ftok->str() != "(") {
                if (ftok->str() == ")")
                    ftok = ftok->link();
                ftok = ftok->previous();
            }
            if (!ftok || !ftok->previous())
                continue;
            std::list<const Token *> varlist;
            parseFunctionCall(*ftok->previous(), varlist, &_settings->library, 0);
            if (std::find(varlist.begin(), varlist.end(), tok) != varlist.end()) {
                if (value->condition == nullptr)
                    nullPointerError(tok, tok->str());
                else if (_settings->isEnabled("warning"))
                    nullPointerError(tok, tok->str(), value->condition, value->inconclusive);
            }
            continue;
        }

        // Pointer dereference.
        bool unknown = false;
        if (!isPointerDeRef(tok,unknown)) {
            if (_settings->inconclusive && unknown) {
                if (value->condition == nullptr)
                    nullPointerError(tok, tok->str(), true);
                else
                    nullPointerError(tok, tok->str(), value->condition, true);
            }
            continue;
        }

        if (value->condition == nullptr)
            nullPointerError(tok, tok->str(), value->inconclusive);
        else if (_settings->isEnabled("warning"))
            nullPointerError(tok, tok->str(), value->condition, value->inconclusive);
    }
}

void CheckNullPointer::nullPointerByCheckAndDeRef()
{
    const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();

    // Check if pointer is NULL and then dereference it..
    for (std::list<Scope>::const_iterator i = symbolDatabase->scopeList.begin(); i != symbolDatabase->scopeList.end(); ++i) {
        if (i->type != Scope::eIf && i->type != Scope::eElseIf && i->type != Scope::eWhile)
            continue;
        if (!i->classDef || i->classDef->isExpandedMacro())
            continue;

        const Token* const tok = i->type != Scope::eElseIf ? i->classDef->next() : i->classDef->tokAt(2);
        // TODO: investigate false negatives:
        // - handle "while"?
        // - if there are logical operators
        // - if (x) { } else { ... }

        // If the if-body ends with a unknown macro then bailout
        if (Token::Match(i->classEnd->tokAt(-3), "[;{}] %var% ;") && i->classEnd->tokAt(-2)->isUpperCaseName())
            continue;

        // vartok : token for the variable
        const Token *vartok = nullptr;
        const Token *checkConditionStart = nullptr;
        if (Token::Match(tok, "( ! %var% )|&&")) {
            vartok = tok->tokAt(2);
            checkConditionStart = vartok->next();
        } else if (Token::Match(tok, "( %var% )|&&")) {
            vartok = tok->next();
        } else if (Token::Match(tok, "( ! ( %var% =")) {
            vartok = tok->tokAt(3);
            if (Token::simpleMatch(tok->linkAt(2), ") &&"))
                checkConditionStart = tok->linkAt(2);
        } else
            continue;

        // Check if variable is a pointer
        const Variable *var = vartok->variable();
        if (!var || !var->isPointer())
            continue;

        // variable id for pointer
        const unsigned int varid(vartok->varId());

        const Scope* declScope = &*i;
        while (declScope->nestedIn && var->scope() != declScope && declScope->type != Scope::eFunction)
            declScope = declScope->nestedIn;

        if (Token::Match(vartok->next(), "&& ( %varid% =", varid))
            continue;

        // Name and line of the pointer
        const std::string &pointerName = vartok->str();

        // Check the condition (eg. ( !x && x->i )
        if (checkConditionStart) {
            const Token * const conditionEnd = tok->link();
            for (const Token *tok2 = checkConditionStart; tok2 != conditionEnd; tok2 = tok2->next()) {
                // If we hit a || operator, abort
                if (tok2->str() == "||")
                    break;

                // Pointer is used
                bool unknown = _settings->inconclusive;
                if (tok2->varId() == varid && (isPointerDeRef(tok2, unknown) || unknown)) {
                    nullPointerError(tok2, pointerName, vartok, unknown);
                    break;
                }
            }
        }

        // start token = inside the if-body
        const Token *tok1 = i->classStart;

        if (Token::Match(tok, "( %var% )|&&")) {
            // start token = first token after the if/while body
            tok1 = i->classEnd->next();
            if (!tok1)
                continue;
        }

        int indentlevel = 0;

        // Set to true if we would normally bail out the check.
        bool inconclusive = false;

        // Count { and } for tok2
        for (const Token *tok2 = tok1; tok2 != declScope->classEnd; tok2 = tok2->next()) {
            if (tok2->str() == "{")
                ++indentlevel;
            else if (tok2->str() == "}") {
                if (indentlevel == 0) {
                    if (_settings->inconclusive)
                        inconclusive = true;
                    else
                        break;
                }
                --indentlevel;

                // calling exit function?
                bool unknown = false;
                if (_tokenizer->IsScopeNoReturn(tok2, &unknown)) {
                    if (_settings->inconclusive && unknown)
                        inconclusive = true;
                    else
                        break;
                }

                if (indentlevel <= 0) {
                    // skip all "else" blocks because they are not executed in this execution path
                    while (Token::simpleMatch(tok2, "} else if ("))
                        tok2 = tok2->linkAt(3)->linkAt(1);
                    if (Token::simpleMatch(tok2, "} else {"))
                        tok2 = tok2->linkAt(2);
                }
            }

            if (tok2->str() == "return" || tok2->str() == "throw") {
                bool unknown = _settings->inconclusive;
                for (; tok2 && tok2->str() != ";"; tok2 = tok2->next()) {
                    if (tok2->varId() == varid) {
                        if (CheckNullPointer::isPointerDeRef(tok2, unknown))
                            nullPointerError(tok2, pointerName, vartok, inconclusive);
                        else if (unknown)
                            nullPointerError(tok2, pointerName, vartok, true);
                        if (Token::Match(tok2, "%var% ?"))
                            break;
                    }
                }
                break;
            }

            // Bailout for "if".
            if (tok2->str() == "if") {
                if (_settings->inconclusive)
                    inconclusive = true;
                else
                    break;
            }

            if (Token::Match(tok2, "goto|continue|break|switch|for"))
                break;

            // parameters to sizeof are not dereferenced
            if (Token::Match(tok2, "decltype|sizeof|typeof")) {
                if (tok2->strAt(1) != "(")
                    tok2 = tok2->next();
                else
                    tok2 = tok2->next()->link();
                continue;
            }

            // function call, check if pointer is dereferenced
            if (Token::Match(tok2, "%var% (") && !Token::Match(tok2, "if|while")) {
                std::list<const Token *> vars;
                parseFunctionCall(*tok2, vars, &_settings->library, 0);
                for (std::list<const Token *>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
                    if (Token::Match(*it, "%varid% [,)]", varid)) {
                        nullPointerError(*it, pointerName, vartok, inconclusive);
                        break;
                    }
                }
            }

            // calling unknown function (abort/init)..
            else if (Token::simpleMatch(tok2, ") ;") &&
                     (Token::Match(tok2->link()->tokAt(-2), "[;{}.] %var% (") ||
                      Token::Match(tok2->link()->tokAt(-5), "[;{}] ( * %var% ) ("))) {
                // noreturn function?
                bool unknown = false;
                if (_tokenizer->IsScopeNoReturn(tok2->tokAt(2), &unknown)) {
                    if (!unknown || !_settings->inconclusive) {
                        break;
                    }
                    inconclusive = _settings->inconclusive;
                }

                // init function (global variables)
                if (!var || !(var->isLocal() || var->isArgument()))
                    break;
            }

            if (tok2->varId() == varid) {
                // unknown: this is set to true by isPointerDeRef if
                //          the function fails to determine if there
                //          is a dereference or not
                bool unknown = _settings->inconclusive;

                if (Token::Match(tok2->previous(), "[;{}=] %var% = 0 ;"))
                    ;

                else if (CheckNullPointer::isPointerDeRef(tok2, unknown))
                    nullPointerError(tok2, pointerName, vartok, inconclusive);

                else if (unknown && _settings->inconclusive)
                    nullPointerError(tok2, pointerName, vartok, true);

                else
                    break;
            }
        }
    }
}


void CheckNullPointer::nullPointer()
{
    nullPointerLinkedList();

    if (_settings->isEnabled("warning")) {
        nullPointerByDeRefAndChec();
        nullPointerByCheckAndDeRef();
        nullPointerDefaultArgument();
    }
}

/** Dereferencing null constant (simplified token list) */
void CheckNullPointer::nullConstantDereference()
{
    const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();

    // THIS ARRAY MUST BE ORDERED ALPHABETICALLY
    static const char* const stl_stream[] = {
        "fstream", "ifstream", "iostream", "istream",
        "istringstream", "stringstream", "wistringstream", "wstringstream"
    };

    const std::size_t functions = symbolDatabase->functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i) {
        const Scope * scope = symbolDatabase->functionScopes[i];
        if (scope->function == 0 || !scope->function->hasBody) // We only look for functions with a body
            continue;

        const Token *tok = scope->classStart;

        if (scope->function && scope->function->isConstructor())
            tok = scope->function->token; // Check initialization list

        for (; tok != scope->classEnd; tok = tok->next()) {
            if (Token::Match(tok, "sizeof|decltype|typeid|typeof ("))
                tok = tok->next()->link();

            else if (Token::simpleMatch(tok, "* 0")) {
                if (Token::Match(tok->previous(), "return|throw|;|{|}|:|[|(|,") || tok->previous()->isOp()) {
                    nullPointerError(tok);
                }
            }

            else if (Token::Match(tok, "0 [") && (tok->previous()->str() != "&" || !Token::Match(tok->next()->link()->next(), "[.(]")))
                nullPointerError(tok);

            else if (Token::Match(tok->previous(), "!!. %var% (") && (tok->previous()->str() != "::" || tok->strAt(-2) == "std")) {
                if (Token::simpleMatch(tok->tokAt(2), "0 )") && tok->varId()) { // constructor call
                    const Variable *var = tok->variable();
                    if (var && !var->isPointer() && !var->isArray() && Token::Match(var->typeStartToken(), "std :: string|wstring !!::"))
                        nullPointerError(tok);
                } else { // function call
                    std::list<const Token *> var;
                    parseFunctionCall(*tok, var, &_settings->library, 0);

                    // is one of the var items a NULL pointer?
                    for (std::list<const Token *>::const_iterator it = var.begin(); it != var.end(); ++it) {
                        if (Token::Match(*it, "0|NULL [,)]")) {
                            nullPointerError(*it);
                        }
                    }
                }
            } else if (Token::Match(tok, "std :: string|wstring ( 0 )"))
                nullPointerError(tok);

            else if (Token::simpleMatch(tok->previous(), ">> 0")) { // Only checking input stream operations is safe here, because otherwise 0 can be an integer as well
                const Token* tok2 = tok->previous(); // Find start of statement
                for (; tok2; tok2 = tok2->previous()) {
                    if (Token::Match(tok2->previous(), ";|{|}|:"))
                        break;
                }
                if (Token::simpleMatch(tok2, "std :: cin"))
                    nullPointerError(tok);
                if (tok2 && tok2->varId() != 0) {
                    const Variable *var = tok2->variable();
                    if (var && var->isStlType(stl_stream))
                        nullPointerError(tok);
                }
            }

            const Variable *ovar = nullptr;
            if (Token::Match(tok, "0 ==|!= %var% !!."))
                ovar = tok->tokAt(2)->variable();
            else if (Token::Match(tok, "%var% ==|!= 0"))
                ovar = tok->variable();
            else if (Token::Match(tok, "%var% =|+ 0 )|]|,|;|+"))
                ovar = tok->variable();
            if (ovar && !ovar->isPointer() && !ovar->isArray() && Token::Match(ovar->typeStartToken(), "std :: string|wstring !!::"))
                nullPointerError(tok);
        }
    }
}

/**
* @brief If tok is a function call that passes in a pointer such that
*         the pointer may be modified, this function will remove that
*         pointer from pointerArgs.
*/
void CheckNullPointer::removeAssignedVarFromSet(const Token* tok, std::set<unsigned int>& pointerArgs)
{
    // If a pointer's address is passed into a function, stop considering it
    if (Token::Match(tok->previous(), "[;{}] %var% (")) {
        // Common functions that are known NOT to modify their pointer argument
        const char safeFunctions[] = "printf|sprintf|fprintf|vprintf";

        const Token* endParen = tok->next()->link();
        for (const Token* tok2 = tok->next(); tok2 != endParen; tok2 = tok2->next()) {
            if (tok2->isName() && tok2->varId() > 0 && !Token::Match(tok, safeFunctions)) {
                pointerArgs.erase(tok2->varId());
            }
        }
    }
}

/**
* @brief Does one part of the check for nullPointer().
* -# default argument that sets a pointer to 0
* -# dereference pointer
*/
void CheckNullPointer::nullPointerDefaultArgument()
{
    const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
    const std::size_t functions = symbolDatabase->functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i) {
        const Scope * scope = symbolDatabase->functionScopes[i];
        if (scope->function == 0 || !scope->function->hasBody) // We only look for functions with a body
            continue;

        // Scan the argument list for default arguments that are pointers and
        // which default to a NULL pointer if no argument is specified.
        std::set<unsigned int> pointerArgs;
        for (const Token *tok = scope->function->arg; tok != scope->function->arg->link(); tok = tok->next()) {

            if (Token::Match(tok, "%var% = 0 ,|)") && tok->varId() != 0) {
                const Variable *var = tok->variable();
                if (var && var->isPointer())
                    pointerArgs.insert(tok->varId());
            }
        }

        // Report an error if any of the default-NULL arguments are dereferenced
        if (!pointerArgs.empty()) {
            for (const Token *tok = scope->classStart; tok != scope->classEnd; tok = tok->next()) {

                // If we encounter a possible NULL-pointer check, skip over its body
                if (Token::simpleMatch(tok, "if ( "))  {
                    bool dependsOnPointer = false;
                    const Token *endOfCondition = tok->next()->link();
                    if (!endOfCondition)
                        continue;

                    const Token *startOfIfBlock =
                        Token::simpleMatch(endOfCondition, ") {") ? endOfCondition->next() : nullptr;
                    if (!startOfIfBlock)
                        continue;

                    // If this if() statement may return, it may be a null
                    // pointer check for the pointers referenced in its condition
                    const Token *endOfIf = startOfIfBlock->link();
                    bool isExitOrReturn =
                        Token::findmatch(startOfIfBlock, "exit|return|throw", endOfIf) != nullptr;

                    if (Token::Match(tok, "if ( %var% == 0 )")) {
                        const unsigned int var = tok->tokAt(2)->varId();
                        if (var > 0 && pointerArgs.count(var) > 0) {
                            if (isExitOrReturn)
                                pointerArgs.erase(var);
                            else
                                dependsOnPointer = true;
                        }
                    } else {
                        for (const Token *tok2 = tok->next(); tok2 != endOfCondition; tok2 = tok2->next()) {
                            if (tok2->isName() && tok2->varId() > 0 &&
                                pointerArgs.count(tok2->varId()) > 0) {

                                // If the if() depends on a pointer and may return, stop
                                // considering that pointer because it may be a NULL-pointer
                                // check that returns if the pointer is NULL.
                                if (isExitOrReturn)
                                    pointerArgs.erase(tok2->varId());
                                else
                                    dependsOnPointer = true;
                            }
                        }
                    }

                    if (dependsOnPointer && endOfIf) {
                        for (; tok != endOfIf; tok = tok->next()) {
                            // If a pointer is assigned a new value, stop considering it.
                            if (Token::Match(tok, "%var% ="))
                                pointerArgs.erase(tok->varId());
                            else
                                removeAssignedVarFromSet(tok, pointerArgs);
                        }
                        continue;
                    }
                }

                // If there is a noreturn function (e.g. exit()), stop considering the rest of
                // this function.
                bool unknown = false;
                if (Token::Match(tok, "return|throw|exit") ||
                    (_tokenizer->IsScopeNoReturn(tok, &unknown) && !unknown))
                    break;

                removeAssignedVarFromSet(tok, pointerArgs);

                if (tok->varId() == 0 || pointerArgs.count(tok->varId()) == 0)
                    continue;

                // If a pointer is assigned a new value, stop considering it.
                if (Token::Match(tok, "%var% ="))
                    pointerArgs.erase(tok->varId());

                // If a pointer dereference is preceded by an && or ||,
                // they serve as a sequence point so the dereference
                // may not be executed.
                if (isPointerDeRef(tok, unknown) && !unknown &&
                    tok->strAt(-1) != "&&" && tok->strAt(-1) != "||" &&
                    tok->strAt(-2) != "&&" && tok->strAt(-2) != "||")
                    nullPointerDefaultArgError(tok, tok->str());
            }
        }
    }
}

void CheckNullPointer::nullPointerError(const Token *tok)
{
    reportError(tok, Severity::error, "nullPointer", "Null pointer dereference");
}

void CheckNullPointer::nullPointerError(const Token *tok, const std::string &varname, bool inconclusive)
{
    reportError(tok, Severity::error, "nullPointer", "Possible null pointer dereference: " + varname, inconclusive);
}

void CheckNullPointer::nullPointerError(const Token *tok, const std::string &varname, const Token* nullCheck, bool inconclusive)
{
    std::list<const Token*> callstack;
    callstack.push_back(tok);
    callstack.push_back(nullCheck);
    const std::string errmsg("Possible null pointer dereference: " + varname + " - otherwise it is redundant to check it against null.");
    reportError(callstack, Severity::warning, "nullPointer", errmsg, inconclusive);
}

void CheckNullPointer::nullPointerDefaultArgError(const Token *tok, const std::string &varname)
{
    reportError(tok, Severity::warning, "nullPointer", "Possible null pointer dereference if the default parameter value is used: " + varname);
}

