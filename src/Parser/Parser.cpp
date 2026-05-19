#include "Parser.hpp"
#include <stdexcept>
using namespace std;

Parser::Parser(Lexer &l) : lexer(l)
{
    curr = lexer.getNextToken();
    while (curr.type == TokenType::COMMENT)
    {
        curr = lexer.getNextToken();
    }
}

Token Parser::advance()
{
    Token t = curr;
    do
    {
        curr = lexer.getNextToken();
    } while (curr.type == TokenType::COMMENT);
    return t;
}

void Parser::expect(TokenType t, const string &msg)
{
    if (curr.type != t)
    {
        string m = "Syntax error: unexpected token ";
        m += (lexer.tokenTypeToString(curr.type) + ", expected " + lexer.tokenTypeToString(t));
        if (!msg.empty())
            m += " (" + msg + ")";
        throw runtime_error(m);
    }
}

ParseNode *Parser::programHeader()
{
    ParseNode *node = new ParseNode("<program-header>");

    if (curr.type == TokenType::PROGRAMSY)
    {
        node->add(new ParseNode("programsy"));
        advance();
        if (curr.type == TokenType::IDENT)
        {
            ParseNode *id = new ParseNode("ident");
            id->token = curr.value;
            node->add(id);
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected program name");
        }

        if (curr.type == TokenType::SEMICOLON)
        {
            node->add(new ParseNode("semicolon"));
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected semicolon after program header");
        }
    }
    else
    {
        throw runtime_error("Syntax error: program must start with 'program'");
    }

    return node;
}

ParseNode *Parser::declarationPart()
{
    ParseNode *node = new ParseNode("<declaration-part>");

    while (curr.type == TokenType::CONSTSY)
    {
        node->add(constDeclaration());
    }

    while (curr.type == TokenType::TYPESY)
    {
        node->add(typeDeclaration());
    }

    while (curr.type == TokenType::VARSY)
    {
        ParseNode *varnode = new ParseNode("<var-declaration>");
        varnode->add(new ParseNode("varsy"));
        advance();

        while (curr.type == TokenType::IDENT)
        {
            ParseNode *idlist = new ParseNode("<identifier-list>");
            ParseNode *id = new ParseNode("ident");
            id->token = curr.value;
            idlist->add(id);
            advance();

            while (curr.type == TokenType::COMMA)
            {
                idlist->add(new ParseNode("comma"));
                advance();

                if (curr.type == TokenType::IDENT)
                {
                    ParseNode *id2 = new ParseNode("ident");
                    id2->token = curr.value;
                    idlist->add(id2);
                    advance();
                }
            }
            varnode->add(idlist);

            if (curr.type == TokenType::COLON)
            {
                varnode->add(new ParseNode("colon"));
                advance();
            }
            else
            {
                throw runtime_error("Syntax error: expected ':' in var declaration");
            }

            varnode->add(typeSpecification());

            if (curr.type == TokenType::SEMICOLON)
            {
                varnode->add(new ParseNode("semicolon"));
                advance();
            }
            else
            {
                throw runtime_error("Syntax error: expected ';' in var declaration");
            }
        }
        node->add(varnode);
    }

    while (curr.type == TokenType::PROCEDURESY || curr.type == TokenType::FUNCTIONSY)
    {
        node->add(subprogramDeclaration());
    }

    return node;
}

ParseNode *Parser::constDeclaration()
{
    ParseNode *node = new ParseNode("<const-declaration>");

    if (curr.type == TokenType::CONSTSY)
    {
        node->add(new ParseNode("constsy"));
        advance();

        while (curr.type == TokenType::IDENT)
        {
            ParseNode *id = new ParseNode("ident");
            id->token = curr.value;
            node->add(id);
            advance();

            if (curr.type == TokenType::EQL || curr.type == TokenType::EQL)
            {
                node->add(new ParseNode("eql"));
                advance();
            }
            node->add(constant());

            if (curr.type == TokenType::SEMICOLON)
            {
                node->add(new ParseNode("semicolon"));
                advance();
            }
            else
                break;
        }
    }
    return node;
}

ParseNode *Parser::constant()
{
    ParseNode *node = new ParseNode("<constant>");

    if (curr.type == TokenType::INTCON || curr.type == TokenType::REALCON || curr.type == TokenType::CHARCON || curr.type == TokenType::STRING)
    {
        ParseNode *n = new ParseNode(lexer.tokenTypeToString(curr.type));
        n->token = curr.value;
        node->add(n);
        advance();
        return node;
    }

    if (curr.type == TokenType::PLUS || curr.type == TokenType::MINUS)
    {
        ParseNode *op = new ParseNode(lexer.tokenTypeToString(curr.type));
        advance();
        node->add(op);

        if (curr.type == TokenType::IDENT || curr.type == TokenType::INTCON || curr.type == TokenType::REALCON)
        {
            ParseNode *n = new ParseNode(lexer.tokenTypeToString(curr.type));
            n->token = curr.value;
            node->add(n);
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected ident, intcon, or realcon after sign in constant");
        }
        return node;
    }

    if (curr.type == TokenType::IDENT)
    {
        ParseNode *n = new ParseNode("ident");
        n->token = curr.value;
        node->add(n);
        advance();
        return node;
    }

    throw runtime_error("Syntax error: expected constant");
}

ParseNode *Parser::typeDeclaration()
{
    ParseNode *node = new ParseNode("<type-declaration>");

    if (curr.type == TokenType::TYPESY)
    {
        node->add(new ParseNode("typesy"));
        advance();

        while (curr.type == TokenType::IDENT)
        {
            ParseNode *id = new ParseNode("ident");
            id->token = curr.value;
            node->add(id);
            advance();

            if (curr.type == TokenType::EQL)
            {
                node->add(new ParseNode("eql"));
                advance();
            }
            node->add(typeSpecification());

            if (curr.type == TokenType::SEMICOLON)
            {
                node->add(new ParseNode("semicolon"));
                advance();
            }
            else
                break;
        }
    }
    return node;
}

ParseNode *Parser::typeSpecification()
{
    ParseNode *node = new ParseNode("<type>");

    if (curr.type == TokenType::ARRAYSY)
    {
        node->add(arrayType());
        return node;
    }
    if (curr.type == TokenType::LPARENT)
    {
        node->add(enumeratedType());
        return node;
    }
    if (curr.type == TokenType::RECORDSY)
    {
        node->add(recordType());
        return node;
    }

    if (curr.type == TokenType::PLUS || curr.type == TokenType::MINUS ||
        curr.type == TokenType::INTCON || curr.type == TokenType::REALCON ||
        curr.type == TokenType::CHARCON || curr.type == TokenType::STRING)
    {
        node->add(rangeType());
        return node;
    }

    if (curr.type == TokenType::IDENT)
    {
        ParseNode *const1 = constant();
        if (curr.type == TokenType::PERIOD)
        {
            ParseNode *rangeNode = new ParseNode("<range>");
            rangeNode->add(const1);

            rangeNode->add(new ParseNode("period"));
            advance();
            if (curr.type == TokenType::PERIOD)
            {
                rangeNode->add(new ParseNode("period"));
                advance();
            }
            else
            {
                throw runtime_error("Syntax error: expected '..' in range");
            }
            rangeNode->add(constant());
            node->add(rangeNode);
        }
        else
        {
            ParseNode *idNode = const1->children[0];
            const1->children.clear();
            delete const1;
            node->add(idNode);
        }
        return node;
    }

    return node;
}

ParseNode *Parser::arrayType()
{
    ParseNode *node = new ParseNode("<array-type>");
    if (curr.type == TokenType::ARRAYSY)
    {
        node->add(new ParseNode("arraysy"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected 'array'");
    }
    if (curr.type == TokenType::LBRACK)
    {
        node->add(new ParseNode("lbrack"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected '[' in array declaration");
    }
    if (curr.type == TokenType::INTCON || curr.type == TokenType::CHARCON || curr.type == TokenType::IDENT || curr.type == TokenType::PLUS || curr.type == TokenType::MINUS)
    {
        node->add(rangeType());
    }
    else
    {
        throw runtime_error("Syntax error: expected range in array declaration");
    }
    if (curr.type == TokenType::RBRACK)
    {
        node->add(new ParseNode("rbrack"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected ']' after array range");
    }
    if (curr.type == TokenType::OFSY)
    {
        node->add(new ParseNode("ofsy"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected 'of' in array declaration");
    }

    node->add(typeSpecification());
    return node;
}

ParseNode *Parser::rangeType()
{
    ParseNode *node = new ParseNode("<range>");
    node->add(constant());
    if (curr.type == TokenType::PERIOD)
    {
        node->add(new ParseNode("period"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected '..' in range");
    }
    if (curr.type == TokenType::PERIOD)
    {
        node->add(new ParseNode("period"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected '..' in range");
    }

    node->add(constant());
    return node;
}

ParseNode *Parser::enumeratedType()
{
    ParseNode *node = new ParseNode("<enumerated>");

    if (curr.type == TokenType::LPARENT)
    {
        node->add(new ParseNode("lparent"));
        advance();

        if (curr.type == TokenType::IDENT)
        {
            ParseNode *id = new ParseNode("ident");
            id->token = curr.value;
            node->add(id);
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected identifier in enumerated type");
        }

        while (curr.type == TokenType::COMMA)
        {
            node->add(new ParseNode("comma"));
            advance();

            if (curr.type == TokenType::IDENT)
            {
                ParseNode *id = new ParseNode("ident");
                id->token = curr.value;
                node->add(id);
                advance();
            }
            else
            {
                throw runtime_error("Syntax error: expected identifier after comma in enumerated type");
            }
        }
        if (curr.type == TokenType::RPARENT)
        {
            node->add(new ParseNode("rparent"));
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected ')' in enumerated type");
        }
    }
    return node;
}

ParseNode *Parser::recordType()
{
    ParseNode *node = new ParseNode("<record-type>");

    if (curr.type == TokenType::RECORDSY)
    {
        node->add(new ParseNode("recordsy"));
        advance();
    }
    node->add(fieldList());
    if (curr.type == TokenType::ENDSY)
    {
        node->add(new ParseNode("endsy"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected 'end' after record fields");
    }

    return node;
}

ParseNode *Parser::fieldList()
{
    ParseNode *node = new ParseNode("<field-list>");
    node->add(fieldPart());

    while (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
        node->add(fieldPart());
    }
    return node;
}

ParseNode *Parser::fieldPart()
{
    ParseNode *node = new ParseNode("<field-part>");
    ParseNode *idlist = new ParseNode("<identifier-list>");

    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        idlist->add(id);
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected identifier in record field");
    }

    while (curr.type == TokenType::COMMA)
    {
        idlist->add(new ParseNode("comma"));
        advance();

        if (curr.type == TokenType::IDENT)
        {
            ParseNode *id = new ParseNode("ident");
            id->token = curr.value;
            idlist->add(id);
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected identifier after comma in record field");
        }
    }
    node->add(idlist);

    if (curr.type == TokenType::COLON)
    {
        node->add(new ParseNode("colon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected ':' in record field");
    }
    node->add(typeSpecification());
    return node;
}

ParseNode *Parser::subprogramDeclaration()
{
    ParseNode *node = new ParseNode("<subprogram-declaration>");

    if (curr.type == TokenType::PROCEDURESY)
    {
        node->add(procedureDeclaration());
    }
    else if (curr.type == TokenType::FUNCTIONSY)
    {
        node->add(functionDeclaration());
    }
    return node;
}

ParseNode *Parser::procedureDeclaration()
{
    ParseNode *node = new ParseNode("<procedure-declaration>");

    if (curr.type == TokenType::PROCEDURESY)
    {
        node->add(new ParseNode("proceduresy"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected 'procedure'");
    }

    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        node->add(id);
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected procedure name");
    }

    if (curr.type == TokenType::LPARENT)
    {
        node->add(formalParameterList());
    }

    if (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected semicolon after procedure header");
    }

    node->add(block());

    if (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected semicolon after procedure block");
    }

    return node;
}

ParseNode *Parser::functionDeclaration()
{
    ParseNode *node = new ParseNode("<function-declaration>");

    if (curr.type == TokenType::FUNCTIONSY)
    {
        node->add(new ParseNode("functionsy"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected 'function'");
    }

    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        node->add(id);
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected function name");
    }

    if (curr.type == TokenType::LPARENT)
    {
        node->add(formalParameterList());
    }

    if (curr.type == TokenType::COLON)
    {
        node->add(new ParseNode("colon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected colon before function return type");
    }

    if (curr.type == TokenType::IDENT)
    {
        ParseNode *returnType = new ParseNode("ident");
        returnType->token = curr.value;
        node->add(returnType);
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected function return type");
    }

    if (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected semicolon after function header");
    }

    node->add(block());

    if (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected semicolon after function block");
    }

    return node;
}

ParseNode *Parser::block()
{
    ParseNode *node = new ParseNode("<block>");

    node->add(declarationPart());
    node->add(compoundStatement());
    return node;
}

ParseNode *Parser::formalParameterList()
{
    ParseNode *node = new ParseNode("<formal-parameter-list>");
    if (curr.type == TokenType::LPARENT)
    {
        node->add(new ParseNode("lparent"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected '(' in formal parameter list");
    }
    node->add(parameterGroup());
    while (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
        node->add(parameterGroup());
    }
    if (curr.type == TokenType::RPARENT)
    {
        node->add(new ParseNode("rparent"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected ')' in formal parameter list");
    }
    return node;
}

ParseNode *Parser::parameterGroup()
{
    ParseNode *node = new ParseNode("<parameter-group>");
    ParseNode *idlist = new ParseNode("<identifier-list>");
    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        idlist->add(id);
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected identifier in parameter group");
    }
    while (curr.type == TokenType::COMMA)
    {
        idlist->add(new ParseNode("comma"));
        advance();
        if (curr.type == TokenType::IDENT)
        {
            ParseNode *id = new ParseNode("ident");
            id->token = curr.value;
            idlist->add(id);
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected identifier after comma in parameter group");
        }
    }
    node->add(idlist);
    if (curr.type == TokenType::COLON)
    {
        node->add(new ParseNode("colon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected ':' in parameter group");
    }
    if (curr.type == TokenType::IDENT || curr.type == TokenType::ARRAYSY)
    {
        node->add(typeSpecification());
    }
    else
    {
        throw runtime_error("Syntax error: expected type specification in parameter group");
    }
    return node;
}

ParseNode *Parser::compoundStatement()
{
    ParseNode *node = new ParseNode("<compound-statement>");

    if (curr.type == TokenType::BEGINSY)
    {
        node->add(new ParseNode("beginsy"));
        advance();
        node->add(statementList());
        if (curr.type == TokenType::ENDSY)
        {
            node->add(new ParseNode("endsy"));
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected 'end'");
        }
    }
    else
    {
        throw runtime_error("Syntax error: expected 'begin'");
    }
    return node;
}

ParseNode *Parser::statementList()
{
    ParseNode *node = new ParseNode("<statement-list>");

    while (curr.type != TokenType::ENDSY && curr.type != TokenType::UNTILSY && curr.type != TokenType::END_OF_FILE)
    {
        node->add(statement());
        if (curr.type == TokenType::SEMICOLON)
        {
            node->add(new ParseNode("semicolon"));
            advance();
        }
        else if (curr.type == TokenType::ENDSY || curr.type == TokenType::UNTILSY || curr.type == TokenType::END_OF_FILE)
        {
            break;
        }
    }
    return node;
}

ParseNode *Parser::statement()
{
    if (curr.type == TokenType::BEGINSY)
        return compoundStatement();
    if (curr.type == TokenType::IFSY)
        return ifStatement();
    if (curr.type == TokenType::CASESY)
        return caseStatement();
    if (curr.type == TokenType::WHILESY)
        return whileStatement();
    if (curr.type == TokenType::REPEATSY)
        return repeatStatement();
    if (curr.type == TokenType::FORSY)
        return forStatement();

    if (curr.type == TokenType::SEMICOLON)
    {
        return new ParseNode("<empty-statement>");
    }

    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        advance();

        if (curr.type == TokenType::LPARENT)
        {
            ParseNode *call = new ParseNode("<procedure/function-call>");
            call->add(id);
            call->add(new ParseNode("lparent"));
            advance();
            if (curr.type != TokenType::RPARENT)
            {
                call->add(parameterList());
            }
            if (curr.type == TokenType::RPARENT)
            {
                call->add(new ParseNode("rparent"));
                advance();
            }
            else
            {
                throw runtime_error("Syntax error: expected ')' after procedure/function call");
            }
            return call;
        }
        else
        {
            ParseNode *varToUse = id;
            if (curr.type == TokenType::LBRACK || curr.type == TokenType::PERIOD)
            {
                ParseNode *var = new ParseNode("<variable>");
                var->add(id);
                while (curr.type == TokenType::LBRACK || curr.type == TokenType::PERIOD)
                {
                    var->add(componentVariable());
                }
                varToUse = var;
            }

            if (curr.type == TokenType::BECOMES)
            {
                ParseNode *node = new ParseNode("<assignment-statement>");
                node->add(varToUse);
                node->add(new ParseNode("becomes"));
                advance();
                node->add(expression());
                return node;
            }
            else if (varToUse == id)
            {
                ParseNode *call = new ParseNode("<procedure/function-call>");
                call->add(id);
                return call;
            }

            throw runtime_error("Syntax error: expected ':=' in assignment");
        }
    }

    ParseNode *node = new ParseNode("<statement>");
    ParseNode *t = new ParseNode(lexer.tokenTypeToString(curr.type));
    t->token = curr.value;
    node->add(t);
    advance();
    return node;
}

ParseNode *Parser::variable()
{
    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        advance();

        // check for components (array index or record field)
        // ident + (component-variable)*
        if (curr.type == TokenType::LBRACK || curr.type == TokenType::PERIOD)
        {
            ParseNode *var = new ParseNode("<variable>");
            var->add(id);

            while (curr.type == TokenType::LBRACK || curr.type == TokenType::PERIOD)
            {
                var->add(componentVariable());
            }

            return var;
        }
        return id;
    }
    throw runtime_error("Syntax error: expected variable (identifier)");
}

ParseNode *Parser::componentVariable()
{
    ParseNode *node = new ParseNode("<component-variable>");

    if (curr.type == TokenType::LBRACK)
    {
        // array index: [index-list]
        node->add(new ParseNode("lbrack"));
        advance();
        node->add(indexList());

        if (curr.type == TokenType::RBRACK)
        {
            node->add(new ParseNode("rbrack"));
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected ']' after index list");
        }
    }
    else if (curr.type == TokenType::PERIOD)
    {
        // .ident
        node->add(new ParseNode("period"));
        advance();
        if (curr.type == TokenType::IDENT)
        {
            ParseNode *id = new ParseNode("ident");
            id->token = curr.value;
            node->add(id);
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected identifier after '.'");
        }
    }
    return node;
}

ParseNode *Parser::indexList()
{
    ParseNode *node = new ParseNode("<index-list>");

    if (curr.type == TokenType::INTCON || curr.type == TokenType::CHARCON || curr.type == TokenType::IDENT)
    {
        ParseNode *n = new ParseNode(lexer.tokenTypeToString(curr.type));
        n->token = curr.value;
        node->add(n);
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected intcon, charcon, or ident in index list");
    }

    while (curr.type == TokenType::COMMA)
    {
        node->add(new ParseNode("comma"));
        advance();
        if (curr.type == TokenType::INTCON || curr.type == TokenType::CHARCON || curr.type == TokenType::IDENT)
        {
            ParseNode *n = new ParseNode(lexer.tokenTypeToString(curr.type));
            n->token = curr.value;
            node->add(n);
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected intcon, charcon, or ident in index list");
        }
    }
    return node;
}

ParseNode *Parser::assignmentStatement()
{
    ParseNode *node = new ParseNode("<assignment-statement>");
    node->add(variable());
    if (curr.type == TokenType::BECOMES)
    {
        node->add(new ParseNode("becomes"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected ':=' in assignment");
    }
    node->add(expression());
    return node;
}

ParseNode *Parser::ifStatement()
{
    ParseNode *node = new ParseNode("<if-statement>");

    if (curr.type == TokenType::IFSY)
    {
        node->add(new ParseNode("ifsy"));
        advance();
    }
    node->add(expression());
    if (curr.type == TokenType::THENSY)
    {
        node->add(new ParseNode("thensy"));
        advance();
    }
    node->add(statement());
    if (curr.type == TokenType::ELSESY)
    {
        node->add(new ParseNode("elsesy"));
        advance();
        node->add(statement());
    }
    return node;
}

ParseNode *Parser::caseBlock()
{
    ParseNode *node = new ParseNode("<case-block>");

    node->add(constant());
    while (curr.type == TokenType::COMMA)
    {
        node->add(new ParseNode("comma"));
        advance();
        node->add(constant());
    }
    if (curr.type == TokenType::COLON)
    {
        node->add(new ParseNode("colon"));
        advance();
    }
    node->add(statement());
    if (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
    }
    return node;
}

ParseNode *Parser::caseStatement()
{
    ParseNode *node = new ParseNode("<case-statement>");

    if (curr.type == TokenType::CASESY)
    {
        node->add(new ParseNode("casesy"));
        advance();
    }
    node->add(expression());
    if (curr.type == TokenType::OFSY)
    {
        node->add(new ParseNode("ofsy"));
        advance();
    }
    while (curr.type != TokenType::ENDSY && curr.type != TokenType::END_OF_FILE)
    {
        node->add(caseBlock());
    }
    if (curr.type == TokenType::ENDSY)
    {
        node->add(new ParseNode("endsy"));
        advance();
    }
    return node;
}

ParseNode *Parser::whileStatement()
{
    ParseNode *node = new ParseNode("<while-statement>");
    if (curr.type == TokenType::WHILESY)
    {
        node->add(new ParseNode("whilesy"));
        advance();
    }
    node->add(expression());
    if (curr.type == TokenType::DOSY)
    {
        node->add(new ParseNode("dosy"));
        advance();
    }
    node->add(compoundStatement());
    if (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected ';' after while statement");
    }
    return node;
}

ParseNode *Parser::repeatStatement()
{
    ParseNode *node = new ParseNode("<repeat-statement>");
    if (curr.type == TokenType::REPEATSY)
    {
        node->add(new ParseNode("repeatsy"));
        advance();
    }
    node->add(statementList());
    if (curr.type == TokenType::UNTILSY)
    {
        node->add(new ParseNode("untilsy"));
        advance();
    }
    node->add(expression());
    return node;
}

ParseNode *Parser::forStatement()
{
    ParseNode *node = new ParseNode("<for-statement>");

    if (curr.type == TokenType::FORSY)
    {
        node->add(new ParseNode("forsy"));
        advance();
    }
    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        node->add(id);
        advance();
    }
    if (curr.type == TokenType::BECOMES)
    {
        node->add(new ParseNode("becomes"));
        advance();
    }
    node->add(expression());
    if (curr.type == TokenType::TOSY)
    {
        node->add(new ParseNode("tosy"));
        advance();
    }
    else if (curr.type == TokenType::DOWNTOSY)
    {
        node->add(new ParseNode("downtosy"));
        advance();
    }
    node->add(expression());
    if (curr.type == TokenType::DOSY)
    {
        node->add(new ParseNode("dosy"));
        advance();
    }
    node->add(compoundStatement());
    if (curr.type == TokenType::SEMICOLON)
    {
        node->add(new ParseNode("semicolon"));
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected ';' after for statement");
    }
    return node;
}

ParseNode *Parser::procedureOrFunctionCall()
{
    ParseNode *node = new ParseNode("<procedure/function-call>");
    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        node->add(id);
        advance();
    }
    else
    {
        throw runtime_error("Syntax error: expected procedure/function name");
    }
    if (curr.type == TokenType::LPARENT)
    {
        node->add(new ParseNode("lparent"));
        advance();
        if (curr.type != TokenType::RPARENT)
        {
            node->add(parameterList());
        }
        if (curr.type == TokenType::RPARENT)
        {
            node->add(new ParseNode("rparent"));
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected ')'");
        }
    }
    return node;
}

ParseNode *Parser::parameterList()
{
    ParseNode *node = new ParseNode("<parameter-list>");
    node->add(expression());
    while (curr.type == TokenType::COMMA)
    {
        node->add(new ParseNode("comma"));
        advance();
        node->add(expression());
    }
    return node;
}

ParseNode *Parser::expression()
{
    ParseNode *node = new ParseNode("<expression>");
    node->add(simpleExpression());

    if (curr.type == TokenType::EQL || curr.type == TokenType::NEQ ||
        curr.type == TokenType::GTR || curr.type == TokenType::GEQ ||
        curr.type == TokenType::LSS || curr.type == TokenType::LEQ)
    {
        node->add(relationalOperator());
        node->add(simpleExpression());
    }
    return node;
}

ParseNode *Parser::simpleExpression()
{
    ParseNode *node = new ParseNode("<simple-expression>");

    if (curr.type == TokenType::PLUS || curr.type == TokenType::MINUS)
    {
        ParseNode *op = new ParseNode(lexer.tokenTypeToString(curr.type));
        op->token = curr.value;
        advance();
        node->add(op);
    }

    node->add(term());

    while (curr.type == TokenType::PLUS || curr.type == TokenType::MINUS || curr.type == TokenType::ORSY)
    {
        node->add(additiveOperator());
        node->add(term());
    }
    return node;
}

ParseNode *Parser::term()
{
    ParseNode *node = new ParseNode("<term>");

    node->add(factor());
    while (curr.type == TokenType::TIMES || curr.type == TokenType::RDIV ||
           curr.type == TokenType::IDIV || curr.type == TokenType::IMOD ||
           curr.type == TokenType::ANDSY)
    {
        node->add(multiplicativeOperator());
        node->add(factor());
    }
    return node;
}

ParseNode *Parser::factor()
{
    ParseNode *node = new ParseNode("<factor>");
    if (curr.type == TokenType::NOTSY)
    {
        node->add(new ParseNode("notsy"));
        advance();
        node->add(factor());
        return node;
    }
    if (curr.type == TokenType::INTCON || curr.type == TokenType::REALCON ||
        curr.type == TokenType::STRING || curr.type == TokenType::CHARCON)
    {
        ParseNode *n = new ParseNode(lexer.tokenTypeToString(curr.type));
        n->token = curr.value;
        node->add(n);
        advance();
        return node;
    }
    if (curr.type == TokenType::LPARENT)
    {
        node->add(new ParseNode("lparent"));
        advance();
        node->add(expression());
        if (curr.type == TokenType::RPARENT)
        {
            node->add(new ParseNode("rparent"));
            advance();
        }
        else
        {
            throw runtime_error("Syntax error: expected )");
        }
        return node;
    }
    if (curr.type == TokenType::IDENT)
    {
        ParseNode *id = new ParseNode("ident");
        id->token = curr.value;
        advance();

        if (curr.type == TokenType::LPARENT)
        {
            ParseNode *call = new ParseNode("<procedure/function-call>");
            call->add(id);
            call->add(new ParseNode("lparent"));
            advance();
            if (curr.type != TokenType::RPARENT)
            {
                call->add(parameterList());
            }
            if (curr.type == TokenType::RPARENT)
            {
                call->add(new ParseNode("rparent"));
                advance();
            }
            else
            {
                throw runtime_error("Syntax error: expected ')' after function call");
            }
            node->add(call);
            return node;
        }
        else
        {
            if (curr.type == TokenType::LBRACK || curr.type == TokenType::PERIOD)
            {
                ParseNode *var = new ParseNode("<variable>");
                var->add(id);
                while (curr.type == TokenType::LBRACK || curr.type == TokenType::PERIOD)
                {
                    var->add(componentVariable());
                }
                node->add(var);
            }
            else
            {
                node->add(id);
            }

            return node;
        }
    }
    // unknown factor
    ParseNode *n = new ParseNode("unknown");
    n->token = curr.value;
    node->add(n);
    advance();
    return node;
}

ParseNode *Parser::relationalOperator()
{
    ParseNode *node = new ParseNode("<relational-operator>");
    if (curr.type == TokenType::EQL || curr.type == TokenType::NEQ || curr.type == TokenType::GTR || curr.type == TokenType::GEQ || curr.type == TokenType::LSS || curr.type == TokenType::LEQ)
    {
        node->add(new ParseNode(lexer.tokenTypeToString(curr.type)));
        advance();
    }
    return node;
}

ParseNode *Parser::additiveOperator()
{
    ParseNode *node = new ParseNode("<additive-operator>");
    if (curr.type == TokenType::PLUS || curr.type == TokenType::MINUS || curr.type == TokenType::ORSY)
    {
        node->add(new ParseNode(lexer.tokenTypeToString(curr.type)));
        advance();
    }
    return node;
}

ParseNode *Parser::multiplicativeOperator()
{
    ParseNode *node = new ParseNode("<multiplicative-operator>");
    if (curr.type == TokenType::TIMES || curr.type == TokenType::RDIV || curr.type == TokenType::IDIV || curr.type == TokenType::IMOD || curr.type == TokenType::ANDSY)
    {
        node->add(new ParseNode(lexer.tokenTypeToString(curr.type)));
        advance();
    }
    return node;
}

ParseNode *Parser::parseProgram()
{
    ParseNode *root = new ParseNode("<program>");
    root->add(programHeader());
    root->add(declarationPart());
    root->add(compoundStatement());
    if (curr.type == TokenType::PERIOD)
    {
        root->add(new ParseNode("period"));
    }
    return root;
}
