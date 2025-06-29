// this file is used for tutorial to build the compiler step by step

#include <unistd.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdint.h>
#define int intptr_t

int token;                    // current token
int token_val;                // value of current token (mainly for number)
char *src, *prev_src;          // pointer to source code string;
int poolsize;                 // default size of text/data/stack
int line_num;                     // line_num number
int *text,                    // text segment
    *prev_text,                // for dump text segment
    *stack;                   // stack
char *data;                   // data segment
int *pc, *bp, *sp, ax;//, cycle; // virtual machine registers
int *curr_id,              // current parsed ID
    *symbol_table;                 // symbol table
int *id_main;                  // the `main` function

// instructions
enum { LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC, MSET,MCMP,EXIT };

// tokens and classes (operators last and in precedence order)
enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// fields of identifier
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};

// types of variable/function
enum { CHAR, INT, PTR };

int base_type;    // the type of a declaration, make it global for convenience
int expr_type;   // the type of an expression

// function frame
//
// 0: arg 1
// 1: arg 2
// 2: arg 3
// 3: return address
// 4: old bp pointer  <- index_of_bp
// 5: local var 1
// 6: local var 2
int index_of_bp; // index of bp pointer on stack

void free_all(){

}

/*
void next_token() {
    char *last_pos;
    int hash;

    while (token = *src) {
        ++src;

        // parse token here
        if (token == '\n') {
            ++line_num;
        }
        else if (token == '#') {
            // skip macro, because we will not support it
            while (*src != 0 && *src != '\n') {
                src++;
            }
        }
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {

            // parse identifier
            last_pos = src - 1;
            hash = token;

            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * 147 + *src;
                src++;
            }

            // look for existing identifier, linear search
            curr_id = symbol_table;
            while (curr_id[Token]) {
                if (curr_id[Hash] == hash && !memcmp((char *)curr_id[Name], last_pos, src - last_pos)) {
                    //found one, return
                    token = curr_id[Token];
                    return;
                }
                curr_id = curr_id + IdSize;
            }


            // store new ID
            curr_id[Name] = (int)last_pos;
            curr_id[Hash] = hash;
            token = curr_id[Token] = Id;
            return;
        }
        else if (token >= '0' && token <= '9') {
            // parse number, three kinds: dec(123) hex(0x123) oct(017)
            token_val = token - '0';
            if (token_val > 0) {
                // dec, starts with [1-9]
                while (*src >= '0' && *src <= '9') {
                    token_val = token_val*10 + *src++ - '0';
                }
            } else {
                // starts with 0
                if (*src == 'x' || *src == 'X') {
                    //hex
                    token = *++src;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')) {
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                } else {
                    // oct
                    while (*src >= '0' && *src <= '7') {
                        token_val = token_val*8 + *src++ - '0';
                    }
                }
            }

            token = Num;
            return;
        }
        else if (token == '"' || token == '\'') {
            // parse string literal, currently, the only supported escape
            // character is '\n', store the string literal into data.
            last_pos = data;
            while (*src != 0 && *src != token) {
                token_val = *src++;
                if (token_val == '\\') {
                    // escape character
                    token_val = *src++;
                    if (token_val == 'n') {
                        token_val = '\n';
                    }
                }

                if (token == '"') {
                    *data++ = token_val;
                }
            }

            src++;
            // if it is a single character, return Num token
            if (token == '"') {
                token_val = (int)last_pos;
            } else {
                token = Num;
            }

            return;
        }
        else if (token == '/') {
            if (*src == '/') {
                // skip comments
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else {
                // divide operator
                token = Div;
                return;
            }
        }
        else if (token == '=') {
            // parse '==' and '='
            if (*src == '=') {
                src ++;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        }
        else if (token == '+') {
            // parse '+' and '++'
            if (*src == '+') {
                src ++;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        }
        else if (token == '-') {
            // parse '-' and '--'
            if (*src == '-') {
                src ++;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        }
        else if (token == '!') {
            // parse '!='
            if (*src == '=') {
                src++;
                token = Ne;
            }
            return;
        }
        else if (token == '<') {
            // parse '<=', '<<' or '<'
            if (*src == '=') {
                src ++;
                token = Le;
            } else if (*src == '<') {
                src ++;
                token = Shl;
            } else {
                token = Lt;
            }
            return;
        }
        else if (token == '>') {
            // parse '>=', '>>' or '>'
            if (*src == '=') {
                src ++;
                token = Ge;
            } else if (*src == '>') {
                src ++;
                token = Shr;
            } else {
                token = Gt;
            }
            return;
        }
        else if (token == '|') {
            // parse '|' or '||'
            if (*src == '|') {
                src ++;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            // parse '&' and '&&'
            if (*src == '&') {
                src ++;
                token = Lan;
            } else {
                token = And;
            }
            return;
        }
        else if (token == '^') {
            token = Xor;
            return;
        }
        else if (token == '%') {
            token = Mod;
            return;
        }
        else if (token == '*') {
            token = Mul;
            return;
        }
        else if (token == '[') {
            token = Brak;
            return;
        }
        else if (token == '?') {
            token = Cond;
            return;
        }
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // directly return the character as token;
            return;
        }
    }
    return;
}
*/

void next_token(){
    // token = *(src++);
    char *last_pos;
    int hash;
    while(token = *src){ // while will skip unknown characters
        src++;
        //identifer and symbol table (uniqe id for var name etc)
        if (token == '\n') {
            ++line_num;
        }
        else if (token == '#') {
            // skip macro, because we will not support it
            while (*src != 0 && *src != '\n') src++;
            
        }
        else if((token >='a' &&token <='z')||(token >='A' &&token <='Z')||token =='_' ){
            // parse id
            last_pos = src -1;
            hash = token;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * 147 + *src; //the hashing algorithm
                src++;
            }
            // look for existing id
            curr_id = symbol_table;
            while(curr_id[Token]){
                
                if(curr_id[Hash]== hash&& !memcmp((char*)curr_id[Name],last_pos,src -last_pos)){
                    token =curr_id[Token];
                    return; 
                }
                curr_id += IdSize;
            }
            // save new id
            curr_id[Name] = (int) last_pos;
            curr_id[Hash] = hash;
            curr_id[Token] = Id;
            token = Id;
            return;
        }
        //numbers suporte: dec(123) hex(0x123) oct(017)
        else if(token >='0' && token<='9'){
            token_val = token -'0';
            if(token_val >0){
                while(*src >= '0' && *src <= '9'){
                    token_val = token_val*10 + *src++ -'0';
                }
            }
            else if(*src =='x' || *src =='X'){
                token = *(++src);
                while((token >= '0' && token <= '9')||(token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')){
                    token_val = token_val*16 + (token & 0x0F) + (token >= 'A' ? 9 : 0);
                    token = *(++src);
                }

            }
            else{
                while(*src >= '0' && *src <= '7'){
                    token_val = token_val*8 + *src++ -'0';
                }
            }
            token = Num;
            return;
        }

        else{
            switch (token)
            {
            // case '\n': 
            //     line_num++;
            //     break;
            // case '#': // skip macros
            //     while(*src != 0 &&*src != '\n') src++;
            //     break;
            case '"': //string literals only '\n' is sopported 
            case '\'':
                last_pos = data;
                while(*src!= 0 && *src != token){
                    token_val = *(src++);
                    if (token_val == '\\') {
                        token_val = *src++;
                        if (token_val == 'n') {
                            token_val = '\n';
                        }
                    }
                    if (token == '"') {
                        *data++ = token_val;
                    }
                }
                src++;
                if (token == '"') {
                    token_val = (int) last_pos;
                } 
                else {
                    token = Num;
                }
                return;
            case '/': 
                if (*src == '/') { // "//" comments (/* ... */ are not supported)
                    while (*src != 0 && *src != '\n') {
                        ++src;
                    }
                    break;
                }
                else { // divide operator
                token = Div;
                return;
                }
                break;
            
            case '=':
                if(*src =='='){
                    src++;
                    token = Eq;
                }
                else{
                    token = Assign;
                }
                return;
            case '+':
                if(*src =='+'){
                    src++;
                    token = Inc;
                }
                else{
                    token = Add;
                }
                return;
            case '-':
                if(*src =='-'){
                    src++;
                    token = Dec;
                }
                else{
                    token = Sub;
                }
                return;
            case '!':
                if(*src =='='){
                    src++;
                    token = Ne;
                }
                return;
            case '<':
                if(*src =='='){
                    src++;
                    token = Le;
                }
                else if(*src == '<'){
                    src++;
                    token = Shl;
                }
                else{
                    token = Lt;
                }
                return;
            case '>':
                if(*src =='='){
                    src++;
                    token = Ge;
                }
                else if(*src == '>'){
                    src++;
                    token = Shr;
                }
                else{
                    token = Gt;
                }
                return;
            case '|':
                if(*src =='|'){
                    src++;
                    token = Lor;
                }
                else{
                    token = Or;
                }
                return;
            case '&':
                if(*src =='&'){
                    src++;
                    token = Lan;
                }
                else{
                    token = And;
                }
                return;
            case '^': token = Xor; return;
            case '%': token = Mod; return;
            case '*': token = Mul; return;
            case '[': token = Brak; return;
            case '?': token = Cond; return;
            case '~':
            case ';':
            case '{':
            case '}':
            case '(':
            case ')':
            case ']':
            case ',':
            case ':':
                return; // the char is return as a  token 
            // default:
            //     break;
            }
        }
    }
    return;
}

void match(int tk) {
    if (token == tk) {
        next_token();
    } else {
        printf("%d: expected token: %d\n", line_num, tk);
        exit(-1);
    }
}
/*
void expression(int level) {
    // expressions have various format.
    // but majorly can be divided into two parts: unit and operator
    // for example `(char) *a[10] = (int *) func(b > 0 ? 10 : 20);
    // `a[10]` is an unit while `*` is an operator.
    // `func(...)` in total is an unit.
    // so we should first parse those unit and unary operators
    // and then the binary ones
    //
    // also the expression can be in the following types:
    //
    // 1. unit_unary ::= unit | unit unary_op | unary_op unit
    // 2. expr ::= unit_unary (bin_op unit_unary ...)

    // unit_unary()
    int *id;
    int tmp;
    int *addr;
    {
        if (!token) {
            printf("%d: unexpected token EOF of expressio n\n", line_num);
            exit(-1);
        }
        if (token == Num) {
            match(Num);

            // emit code
            *++text = IMM;
            *++text = token_val;
            expr_type = INT;
        }
        else if (token == '"') {
            // continous string "abc" "abc"


            // emit code
            *++text = IMM;
            *++text = token_val;

            match('"');
            // store the rest strings
            while (token == '"') {
                match('"');
            }

            // append the end of string character '\0', all the data are default
            // to 0, so just move data one position forward.
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));
            expr_type = PTR;
        }
        else if (token == Sizeof) {
            // sizeof is actually an unary operator
            // now only `sizeof(int)`, `sizeof(char)` and `sizeof(*...)` are
            // supported.
            match(Sizeof);
            match('(');
            expr_type = INT;

            if (token == Int) {
                match(Int);
            } else if (token == Char) {
                match(Char);
                expr_type = CHAR;
            }

            while (token == Mul) {
                match(Mul);
                expr_type = expr_type + PTR;
            }

            match(')');

            // emit code
            *++text = IMM;
            *++text = (expr_type == CHAR) ? sizeof(char) : sizeof(int);

            expr_type = INT;
        }
        else if (token == Id) {
            // there are several type when occurs to Id
            // but this is unit, so it can only be
            // 1. function call
            // 2. Enum variable
            // 3. global/local variable
            match(Id);

            id = curr_id;

            if (token == '(') {
                // function call
                match('(');

                // pass in arguments
                tmp = 0; // number of arguments
                while (token != ')') {
                    expression(Assign);
                    *++text = PUSH;
                    tmp ++;

                    if (token == ',') {
                        match(',');
                    }

                }
                match(')');

                // emit code
                if (id[Class] == Sys) {
                    // system functions
                    *++text = id[Value];
                }
                else if (id[Class] == Fun) {
                    // function call
                    *++text = CALL;
                    *++text = id[Value];
                }
                else {
                    printf("%d: bad function call\n", line_num);
                    exit(-1);
                }

                // clean the stack for arguments
                if (tmp > 0) {
                    *++text = ADJ;
                    *++text = tmp;
                }
                expr_type = id[Type];
            }
            else if (id[Class] == Num) {
                // enum variable
                *++text = IMM;
                *++text = id[Value];
                expr_type = INT;
            }
            else {
                // variable
                if (id[Class] == Loc) {
                    *++text = LEA;
                    *++text = index_of_bp - id[Value];
                }
                else if (id[Class] == Glo) {
                    *++text = IMM;
                    *++text = id[Value];
                }
                else {
                    printf("%d: undefined variable\n", line_num);
                    exit(-1);
                }

                // emit code, default behaviour is to load the value of the
                // address which is stored in `ax`
                expr_type = id[Type];
                *++text = (expr_type == CHAR) ? LC : LI;
            }
        }
        else if (token == '(') {
            // cast or parenthesis
            match('(');
            if (token == Int || token == Char) {
                tmp = (token == Char) ? CHAR : INT; // cast type
                match(token);
                while (token == Mul) {
                    match(Mul);
                    tmp = tmp + PTR;
                }

                match(')');

                expression(Inc); // cast has precedence as Inc(++)

                expr_type  = tmp;
            } else {
                // normal parenthesis
                expression(Assign);
                match(')');
            }
        }
        else if (token == Mul) {
            // dereference *<addr>
            match(Mul);
            expression(Inc); // dereference has the same precedence as Inc(++)

            if (expr_type >= PTR) {
                expr_type = expr_type - PTR;
            } else {
                printf("%d: bad dereference\n", line_num);
                exit(-1);
            }

            *++text = (expr_type == CHAR) ? LC : LI;
        }
        else if (token == And) {
            // get the address of
            match(And);
            expression(Inc); // get the address of
            if (*text == LC || *text == LI) {
                text --;
            } else {
                printf("%d: bad address of\n", line_num);
                exit(-1);
            }

            expr_type = expr_type + PTR;
        }
        else if (token == '!') {
            // not
            match('!');
            expression(Inc);

            // emit code, use <expr> == 0
            *++text = PUSH;
            *++text = IMM;
            *++text = 0;
            *++text = EQ;

            expr_type = INT;
        }
        else if (token == '~') {
            // bitwise not
            match('~');
            expression(Inc);

            // emit code, use <expr> XOR -1
            *++text = PUSH;
            *++text = IMM;
            *++text = -1;
            *++text = XOR;

            expr_type = INT;
        }
        else if (token == Add) {
            // +var, do nothing
            match(Add);
            expression(Inc);

            expr_type = INT;
        }
        else if (token == Sub) {
            // -var
            match(Sub);

            if (token == Num) {
                *++text = IMM;
                *++text = -token_val;
                match(Num);
            } else {

                *++text = IMM;
                *++text = -1;
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
            }

            expr_type = INT;
        }
        else if (token == Inc || token == Dec) {
            tmp = token;
            match(token);
            expression(Inc);
            if (*text == LC) {
                *text = PUSH;  // to duplicate the address
                *++text = LC;
            } else if (*text == LI) {
                *text = PUSH;
                *++text = LI;
            } else {
                printf("%d: bad lvalue of pre-increment\n", line_num);
                exit(-1);
            }
            *++text = PUSH;
            *++text = IMM;
            *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
            *++text = (tmp == Inc) ? ADD : SUB;
            *++text = (expr_type == CHAR) ? SC : SI;
        }
        else {
            printf("%d: bad expression\n", line_num);
            exit(-1);
        }
    }

    // binary operator and postfix operators.
    {
        while (token >= level) {
            // handle according to current operator's precedence
            tmp = expr_type;
            if (token == Assign) {
                // var = expr;
                match(Assign);
                if (*text == LC || *text == LI) {
                    *text = PUSH; // save the lvalue's pointer
                } else {
                    printf("%d: bad lvalue in assignment\n", line_num);
                    exit(-1);
                }
                expression(Assign);

                expr_type = tmp;
                *++text = (expr_type == CHAR) ? SC : SI;
            }
            else if (token == Cond) {
                // expr ? a : b;
                match(Cond);
                *++text = JZ;
                addr = ++text;
                expression(Assign);
                if (token == ':') {
                    match(':');
                } else {
                    printf("%d: missing colon in conditional\n", line_num);
                    exit(-1);
                }
                *addr = (int)(text + 3);
                *++text = JMP;
                addr = ++text;
                expression(Cond);
                *addr = (int)(text + 1);
            }
            else if (token == Lor) {
                // logic or
                match(Lor);
                *++text = JNZ;
                addr = ++text;
                expression(Lan);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if (token == Lan) {
                // logic and
                match(Lan);
                *++text = JZ;
                addr = ++text;
                expression(Or);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if (token == Or) {
                // bitwise or
                match(Or);
                *++text = PUSH;
                expression(Xor);
                *++text = OR;
                expr_type = INT;
            }
            else if (token == Xor) {
                // bitwise xor
                match(Xor);
                *++text = PUSH;
                expression(And);
                *++text = XOR;
                expr_type = INT;
            }
            else if (token == And) {
                // bitwise and
                match(And);
                *++text = PUSH;
                expression(Eq);
                *++text = AND;
                expr_type = INT;
            }
            else if (token == Eq) {
                // equal ==
                match(Eq);
                *++text = PUSH;
                expression(Ne);
                *++text = EQ;
                expr_type = INT;
            }
            else if (token == Ne) {
                // not equal !=
                match(Ne);
                *++text = PUSH;
                expression(Lt);
                *++text = NE;
                expr_type = INT;
            }
            else if (token == Lt) {
                // less than
                match(Lt);
                *++text = PUSH;
                expression(Shl);
                *++text = LT;
                expr_type = INT;
            }
            else if (token == Gt) {
                // greater than
                match(Gt);
                *++text = PUSH;
                expression(Shl);
                *++text = GT;
                expr_type = INT;
            }
            else if (token == Le) {
                // less than or equal to
                match(Le);
                *++text = PUSH;
                expression(Shl);
                *++text = LE;
                expr_type = INT;
            }
            else if (token == Ge) {
                // greater than or equal to
                match(Ge);
                *++text = PUSH;
                expression(Shl);
                *++text = GE;
                expr_type = INT;
            }
            else if (token == Shl) {
                // shift left
                match(Shl);
                *++text = PUSH;
                expression(Add);
                *++text = SHL;
                expr_type = INT;
            }
            else if (token == Shr) {
                // shift right
                match(Shr);
                *++text = PUSH;
                expression(Add);
                *++text = SHR;
                expr_type = INT;
            }
            else if (token == Add) {
                // add
                match(Add);
                *++text = PUSH;
                expression(Mul);

                expr_type = tmp;
                if (expr_type > PTR) {
                    // pointer type, and not `char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                *++text = ADD;
            }
            else if (token == Sub) {
                // sub
                match(Sub);
                *++text = PUSH;
                expression(Mul);
                if (tmp > PTR && tmp == expr_type) {
                    // pointer subtraction
                    *++text = SUB;
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = DIV;
                    expr_type = INT;
                } else if (tmp > PTR) {
                    // pointer movement
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                    *++text = SUB;
                    expr_type = tmp;
                } else {
                    // numeral subtraction
                    *++text = SUB;
                    expr_type = tmp;
                }
            }
            else if (token == Mul) {
                // multiply
                match(Mul);
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
                expr_type = tmp;
            }
            else if (token == Div) {
                // divide
                match(Div);
                *++text = PUSH;
                expression(Inc);
                *++text = DIV;
                expr_type = tmp;
            }
            else if (token == Mod) {
                // Modulo
                match(Mod);
                *++text = PUSH;
                expression(Inc);
                *++text = MOD;
                expr_type = tmp;
            }
            else if (token == Inc || token == Dec) {
                // postfix inc(++) and dec(--)
                // we will increase the value to the variable and decrease it
                // on `ax` to get its original value.
                if (*text == LI) {
                    *text = PUSH;
                    *++text = LI;
                }
                else if (*text == LC) {
                    *text = PUSH;
                    *++text = LC;
                }
                else {
                    printf("%d: bad value in increment\n", line_num);
                    exit(-1);
                }

                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? ADD : SUB;
                *++text = (expr_type == CHAR) ? SC : SI;
                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? SUB : ADD;
                match(token);
            }
            else if (token == Brak) {
                // array access var[xx]
                match(Brak);
                *++text = PUSH;
                expression(Assign);
                match(']');

                if (tmp > PTR) {
                    // pointer, `not char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                else if (tmp < PTR) {
                    printf("%d: pointer type expected\n", line_num);
                    exit(-1);
                }
                expr_type = tmp - PTR;
                *++text = ADD;
                *++text = (expr_type == CHAR) ? LC : LI;
            }
            else {
                printf("%d: compiler error, token = %d\n", line_num, token);
                exit(-1);
            }
        }
    }
}
*/

void expression(int level) {
    int *id;
    int tmp;
    int *addr;
    if (!token) {
        printf("ERROR in line %d: unexpected token EOF of expression\n", line_num);
        free_all();
        exit(-1);
    }
    switch(token){
        case Num:
            match(Num);
            *(++text) = IMM;
            *(++text) = token_val;
            expr_type = INT;
            break;
        case '"':
            // emit code
            *(++text) = IMM;
            *(++text) = token_val;
            match('"');

            // store the rest strings
            while (token == '"') match('"');

            //This line is ensuring that the data pointer is always aligned to an int boundary
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int))); 
            expr_type = PTR;  
            break;
        case Sizeof:
            match(Sizeof);
            match('(');
            expr_type = INT;

            if (token == Int) {
                match(Int);
            } else if (token == Char) {
                match(Char);
                expr_type = CHAR;
            }

            while (token == Mul) {
                match(Mul);
                expr_type = expr_type + PTR;
            }

            match(')');

            // emit code
            *++text = IMM;
            *++text = (expr_type == CHAR) ? sizeof(char) : sizeof(int);
            expr_type = INT;
            break;
        case Id:
            // 1. function call
            // 2. Enum variable
            // 3. global/local variable
            match(Id);

            id = curr_id;

            if (token == '(') {
                // function call
                match('(');
                // pass in arguments
                tmp = 0; // number of arguments
                while (token != ')') {
                    expression(Assign);
                    *(++text) = PUSH;
                    tmp++;

                    if (token == ',') match(',');
                }
                match(')');

                if (id[Class] == Sys) {
                    // system functions
                    *(++text) = id[Value];
                }
                else if (id[Class] == Fun) {
                    // function call
                    *(++text) = CALL;
                    *(++text) = id[Value];
                }
                else {
                    printf("ERROR in line %d: bad function call\n", line_num);
                    free_all();
                    exit(-1);
                }

            
                // clean the stack for arguments
                if (tmp > 0) {
                    *(++text) = ADJ;
                    *(++text) = tmp;
                }
                expr_type = id[Type];
            }
            else if (id[Class] == Num) {
                // enum variable
                *(++text) = IMM;
                *(++text) = id[Value];
                expr_type = INT;
            }
            else {
                // variable
                if (id[Class] == Loc) {
                    *(++text) = LEA;
                    *(++text) = index_of_bp - id[Value];
                }
                else if (id[Class] == Glo) {
                    *(++text) = IMM;
                    *(++text) = id[Value];
                }
                else {
                    printf("ERROR in line %d: undefined variable\n", line_num);
                    free_all();
                    exit(-1);
                }
                // emit code, default behaviour is to load the value of the
                // address which is stored in `ax`
                expr_type = id[Type];
                *++text = (expr_type == Char) ? LC : LI;
            }
            break;
        case '(':
            // cast or parenthesis
            match('(');
            if (token == Int || token == Char) {
                tmp = (token == Char) ? CHAR : INT; // cast type
                match(token);
                while (token == Mul) {
                    match(Mul);
                    tmp = tmp + PTR;
                }
                match(')');

                expression(Inc); // cast has precedence as Inc(++)

                expr_type  = tmp;
            } 
            else {
                // normal parenthesis
                expression(Assign);
                match(')');
            }
            break;
        case Mul:
            // dereference *<addr>
            match(Mul);
            expression(Inc); // dereference has the same precedence as Inc(++)

            if (expr_type >= PTR) {
                expr_type = expr_type - PTR;
            } 
            else {
                printf("ERROR in line %d: bad dereference\n", line_num);
                exit(-1);
            }

            *(++text) = (expr_type == CHAR) ? LC : LI;
            break;
        case And:
            // get the address of
            match(And);
            expression(Inc); // get the address of
            if (*text == LC || *text == LI) {
                text--;
            } else {
                printf("ERROR in line %d: bad address of\n", line_num);
                exit(-1);
            }

            expr_type = expr_type + PTR;
            break;
        case '!':
            match('!');
            expression(Inc);

            // emit code, use <expr> == 0
            *(++text) = PUSH;
            *(++text) = IMM;
            *(++text) = 0;
            *(++text)= EQ;

            expr_type = INT;
            break;
        case '~':
            // bitwise not
            match('~');
            expression(Inc);

            // emit code, use <expr> XOR -1
            *(++text) = PUSH;
            *(++text)= IMM;
            *(++text)= -1;
            *(++text)= XOR;

            expr_type = INT;
            break;
        case Add:
            match(Add);
            expression(Inc);
            expr_type = INT;
            break;
        case Sub:
            match(Sub);
            if (token == Num) {
                *(++text) = IMM;
                *(++text) = -token_val;
                match(Num);
            } 
            else {
                *(++text) = IMM;
                *(++text) = -1;
                *(++text) = PUSH;
                expression(Inc);
                *(++text) = MUL;
            }
            expr_type = INT;
            break;
        case Inc:
        case Dec:
            tmp = token;
            match(token);
            expression(Inc);
            if (*text == LC) {
                *text = PUSH;  // to duplicate the address
                *++text = LC;
            } 
            else if (*text == LI) {
                *text = PUSH;
                *++text = LI;
            } 
            else {
                printf("ERROR in line %d: invalid lvalue of pre-increment\n", line_num);
                free_all();
                exit(-1);
            }
            *++text = PUSH;
            *++text = IMM;

            *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
            *++text = (tmp == Inc) ? ADD : SUB;
            *++text = (expr_type == CHAR) ? SC : SI;
        break;
        default:
            printf("ERROR in line %d: invalid expression\n", line_num);
            free_all();
            exit(-1);
    }
    // binary operator and postfix operators.
    while(token>=level){
        tmp = expr_type;
        switch(token){
            case Assign:
                // var = expr;
                match(Assign);
                if (*text == LC || *text == LI) *text = PUSH; // save the lvalue's pointer
                else {
                    printf("ERROR in line %d: invalid lvalue in assignment\n", line_num);
                    free_all();
                    exit(-1);
                }
                expression(Assign);

                expr_type = tmp;
                *++text = (expr_type == CHAR) ? SC : SI;
                break;
            case Cond:
                // expr ? a : b;
                match(Cond);
                *++text = JZ;
                addr = ++text;
                expression(Assign);
                if (token == ':') match(':'); 
                else {
                    printf("ERROR in line %d: missing colon in conditional\n", line_num);
                    free_all();
                    exit(-1);
                }
                *addr = (int)(text + 3);
                *++text = JMP;
                addr = ++text;
                expression(Cond);
                *addr = (int)(text + 1);
                break;
            case Lor:
                match(Lor);
                *++text = JNZ;
                addr = ++text;
                expression(Lan);
                *addr = (int)(text + 1);
                expr_type = INT;
                break;
            case Lan:
                match(Lan);
                *++text = JZ;
                addr = ++text;
                expression(Or);
                *addr = (int)(text + 1);
                expr_type = INT;
                break;
            case Xor:
                // bitwise xor
                match(Xor);
                *++text = PUSH;
                expression(And);
                *++text = XOR;
                expr_type = INT;  
                break;
            case Or: 
                match(Or);
                *++text = PUSH;
                expression(Xor);
                *++text = OR;
                expr_type = INT;            
            break;
            
            case And: 
                match(And);
                *++text = PUSH;
                expression(Eq);
                *++text = AND;
                expr_type = INT;            
            break;
            
            case Eq: 
                match(Eq);
                *++text = PUSH;
                expression(Ne);
                *++text = EQ;
                expr_type = INT;            
                break;
            case Ne: 
                 match(Ne);
                *++text = PUSH;
                expression(Lt);
                *++text = NE;
                expr_type = INT;           
                break;
            case Lt:
                match(Lt);
                *++text = PUSH;
                expression(Shl);
                *++text = LT;
                expr_type = INT;                
                break;

            case Gt:
                 match(Gt);
                *++text = PUSH;
                expression(Shl);
                *++text = GT;
                expr_type = INT;           
                break;
            case Le:
                match(Le);
                *++text = PUSH;
                expression(Shl);
                *++text = LE;
                expr_type = INT;            
                break;
            case Ge:
                 match(Ge);
                *++text = PUSH;
                expression(Shl);
                *++text = GE;
                expr_type = INT;           
                break;
            case Shl:
                match(Shl);
                *++text = PUSH;
                expression(Add);
                *++text = SHL;
                expr_type = INT;            
                break;
            case Shr:
                // shift right
                match(Shr);
                *++text = PUSH;
                expression(Add);
                *++text = SHR;
                expr_type = INT;
                break;         
            case Add:
                match(Add);
                *++text = PUSH;
                expression(Mul);
                expr_type = tmp;

                if (expr_type > PTR) {
                    // pointer type, and not `char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                *++text = ADD;
            break;
            case Sub:
                match(Sub);
                *++text = PUSH;
                expression(Mul);
                if (tmp > PTR && tmp == expr_type) {
                    // pointer subtraction
                    *++text = SUB;
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = DIV;
                    expr_type = INT;
                } 
                else if (tmp > PTR) {
                    // pointer movement
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                    *++text = SUB;
                    expr_type = tmp;
                } 
                else {
                    // numeral subtraction
                    *++text = SUB;
                    expr_type = tmp;
                }
                break;
            case Mul:
                match(Mul);
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
                expr_type = tmp;
            break;

            case Div:
                match(Div);
                *++text = PUSH;
                expression(Inc);
                *++text = DIV;
                expr_type = tmp;
                break;
            case Mod:
                match(Mod);
                *++text = PUSH;
                expression(Inc);
                *++text = MOD;
                expr_type = tmp;            
            break;

            case Inc:
            case Dec:
                // postfix inc(++) and dec(--)
                // we will increase the value to the variable and decrease it
                // on `ax` to get its original value.
                if (*text == LI) {
                    *text = PUSH;
                    *++text = LI;
                }
                else if (*text == LC) {
                    *text = PUSH;
                    *++text = LC;
                }
                else {
                    printf("ERROR in line %d: invalid value in increment\n", line_num);
                    free_all();
                    exit(-1);
                }

                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? ADD : SUB;
                *++text = (expr_type == CHAR) ? SC : SI;
                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? SUB : ADD;
                match(token);                
                break;
            case Brak: //[]
                match(Brak);
                *++text = PUSH;
                expression(Assign);
                match(']');

                if (tmp > PTR) {
                    // pointer, `not char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                else if (tmp < PTR) {
                    printf("ERROR in line %d: pointer type expected\n", line_num);
                    free_all();
                    exit(-1);
                }
                expr_type = tmp - PTR;
                *++text = ADD;
                *++text = (expr_type == CHAR) ? LC : LI;                
                break;
            default:
                    printf("ERROR in line %d: compiler error, token = %d\n", line_num,token);
                    free_all();
                    exit(-1);
        }
    }

}

/*void statement() {
    // there are 6 kinds of statements here:
    // 1. if (...) <statement> [else <statement>]
    // 2. while (...) <statement>
    // 3. { <statement> }
    // 4. return xxx;
    // 5. <empty statement>;
    // 6. expression; (expression end with semicolon)

    int *a, *b; // bess for branch control

    if (token == If) {
        // if (...) <statement> [else <statement>]
        //
        //   if (...)           <cond>
        //                      JZ a
        //     <statement>      <statement>
        //   else:              JMP b
        // a:                 a:
        //     <statement>      <statement>
        // b:                 b:
        //
        //
        match(If);
        match('(');
        expression(Assign);  // parse condition
        match(')');

        // emit code for if
        *++text = JZ;
        b = ++text;

        statement();         // parse statement
        if (token == Else) { // parse else
            match(Else);

            // emit code for JMP B
            *b = (int)(text + 3);
            *++text = JMP;
            b = ++text;

            statement();
        }

        *b = (int)(text + 1);
    }
    else if (token == While) {
        //
        // a:                     a:
        //    while (<cond>)        <cond>
        //                          JZ b
        //     <statement>          <statement>
        //                          JMP a
        // b:                     b:
        match(While);

        a = text + 1;

        match('(');
        expression(Assign);
        match(')');

        *++text = JZ;
        b = ++text;

        statement();

        *++text = JMP;
        *++text = (int)a;
        *b = (int)(text + 1);
    }
    else if (token == '{') {
        // { <statement> ... }
        match('{');

        while (token != '}') {
            statement();
        }

        match('}');
    }
    else if (token == Return) {
        // return [expression];
        match(Return);

        if (token != ';') {
            expression(Assign);
        }

        match(';');

        // emit code for return
        *++text = LEV;
    }
    else if (token == ';') {
        // empty statement
        match(';');
    }
    else {
        // a = b; or function_call();
        expression(Assign);
        match(';');
    }
}
*/

void statement() {
    // there are 6 kinds of statements here:
    // 1. if (...) <statement> [else <statement>]
    // 2. while (...) <statement>
    // 3. { <statement> }
    // 4. return xxx;
    // 5. <empty statement>;
    int *a = NULL;
    int* b = NULL;
    switch (token){
        case If:
            match(If);
            match('(');
            expression(Assign);  // parse condition
            match(')');

            *(++text) = JZ;
            b = ++text;

            statement();         // parse statement
            if (token == Else) { // parse else
                match(Else);

                // emit code for JMP B
                *b = (int)(text + 3);
                *(++text) = JMP;
                b = ++text;

                statement();
            }

            *b = (int)(text + 1);
            break;
        case While:
            match(While);

            a = text + 1;

            match('(');
            expression(Assign);
            match(')');

            *(++text) = JZ;
            b = ++text;

            statement();

            *(++text) = JMP;
            *(++text) = (int)a;
            *b = (int)(text + 1);

            break;
        case Return:
            match(Return);

            if (token != ';') expression(Assign);
            match(';');

            // emit code for return
            *(++text) = LEV;
            break;
        case '{':
            // { <statement> ... }
            match('{');
            while (token != '}') statement();
            match('}');
            break;
        case ';':
            match(';');
            break;
        default:
            // a = b; or function_call();
            expression(Assign);
            match(';');
    }
}


/*
void function_parameter() {
    int type;
    int params;
    params = 0;
    while (token != ')') {
        // int name, ...
        type = INT;
        if (token == Int) {
            match(Int);
        } else if (token == Char) {
            type = CHAR;
            match(Char);
        }

        // pointer type
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }

        // parameter name
        if (token != Id) {
            printf("%d: bad parameter declaration\n", line_num);
            exit(-1);
        }
        if (curr_id[Class] == Loc) {
            printf("%d: duplicate parameter declaration\n", line_num);
            exit(-1);
        }

        match(Id);
        // store the local variable
        curr_id[BClass] = curr_id[Class]; curr_id[Class]  = Loc;
        curr_id[BType]  = curr_id[Type];  curr_id[Type]   = type;
        curr_id[BValue] = curr_id[Value]; curr_id[Value]  = params++;   // index of current parameter

        if (token == ',') {
            match(',');
        }
    }
    index_of_bp = params+1;
}

void function_body() {
    // type func_name (...) {...}
    //                   -->|   |<--

    // ... {
    // 1. local declarations
    // 2. statements
    // }

    int pos_local; // position of local variables on the stack.
    int type;
    pos_local = index_of_bp;

    while (token == Int || token == Char) {
        // local variable declaration, just like global ones.
        base_type = (token == Int) ? INT : CHAR;
        match(token);

        while (token != ';') {
            type = base_type;
            while (token == Mul) {
                match(Mul);
                type = type + PTR;
            }

            if (token != Id) {
                // invalid declaration
                printf("%d: bad local declaration\n", line_num);
                exit(-1);
            }
            if (curr_id[Class] == Loc) {
                // identifier exists
                printf("%d: duplicate local declaration\n", line_num);
                exit(-1);
            }
            match(Id);

            // store the local variable
            curr_id[BClass] = curr_id[Class]; curr_id[Class]  = Loc;
            curr_id[BType]  = curr_id[Type];  curr_id[Type]   = type;
            curr_id[BValue] = curr_id[Value]; curr_id[Value]  = ++pos_local;   // index of current parameter

            if (token == ',') {
                match(',');
            }
        }
        match(';');
    }

    // save the stack size for local variables
    *++text = ENT;
    *++text = pos_local - index_of_bp;

    // statements
    while (token != '}') {
        statement();
    }

    // emit code for leaving the sub function
    *++text = LEV;
}

void function_declaration() {
    // type func_name (...) {...}
    //               | this part

    match('(');
    function_parameter();
    match(')');
    match('{');
    function_body();
    //match('}');

    // unwind local variable declarations for all local variables.
    curr_id = symbol_table;
    while (curr_id[Token]) {
        if (curr_id[Class] == Loc) {
            curr_id[Class] = curr_id[BClass];
            curr_id[Type]  = curr_id[BType];
            curr_id[Value] = curr_id[BValue];
        }
        curr_id = curr_id + IdSize;
    }
}
*/


void function_parameters() {
    int type=0;
    int params =0;
    
    while (token != ')') {
        // int name, ...
        type = INT;
        if (token == Int) {
            match(Int);
        } else if (token == Char) {
            type = CHAR;
            match(Char);
        }

        // pointer type
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }

        // parameter name
        if (token != Id) {
            printf("ERROR in line %d: invalid parameter declaration\n", line_num);
            exit(-1);
        }
        if (curr_id[Class] == Loc) {
            printf("ERROR in line %d: duplicate parameter declaration\n", line_num);
            exit(-1);
        }

        match(Id);

        // store the local variable
        curr_id[BClass] = curr_id[Class]; 
        curr_id[Class]  = Loc;
        curr_id[BType]  = curr_id[Type];  
        curr_id[Type]   = type;
        curr_id[BValue] = curr_id[Value]; 
        curr_id[Value]  = params++;   // index of current parameter

        if (token == ',') {
            match(',');
        }
    }
    index_of_bp = params+1;
}

void function_body() {


    int pos_local=0; // position of local variables on the stack.
    int type=0;
    pos_local = index_of_bp;

    while (token == Int || token == Char) {
        // local variable declaration, just like global ones.
        base_type = (token == Int) ? INT : CHAR;
        match(token);

        while (token != ';') {
            type = base_type;
            while (token == Mul) {
                match(Mul);
                type = type + PTR;
            }

            if (token != Id) {
                // invalid declaration
                printf("ERROR in line %d: invalid local declaration\n", line_num);
                free_all();
                exit(-1);
            }
            if (curr_id[Class] == Loc) {
                // identifier exists 
                printf("ERROR in line %d: duplicate local declaration\n", line_num);
                free_all();
                exit(-1);
            }
            match(Id);

            // store the local variable
            curr_id[BClass] = curr_id[Class]; 
            curr_id[Class]  = Loc;
            curr_id[BType]  = curr_id[Type];
            curr_id[Type]   = type;
            curr_id[BValue] = curr_id[Value]; 
            curr_id[Value]  = ++pos_local;   // index of current parameter

            if (token == ',') {
                match(',');
            }
        }
        match(';');
    }


    *(++text) = ENT;
    *(++text) = pos_local - index_of_bp;

    while (token != '}') {
        statement();
    }

    // emit code for leaving the sub function
    *(++text) = LEV;
}

void function_declaration(){ // type func_name (...) {...}
    match('(');
    function_parameters();
    match(')');
    match('{');
    function_body();

    curr_id = symbol_table;
    while (curr_id[Token]) {
        if (curr_id[Class] == Loc) {
            curr_id[Class] = curr_id[BClass];
            curr_id[Type]  = curr_id[BType];
            curr_id[Value] = curr_id[BValue];
        }
        curr_id = curr_id + IdSize;
    }


}

/*
void enum_declaration() {
    // parse enum [id] { a = 1, b = 3, ...}
    int i;
    i = 0;
    while (token != '}') {
        if (token != Id) {
            printf("%d: bad enum identifier %d\n", line_num, token);
            exit(-1);
        }
        next_token();
        if (token == Assign) {
            // like {a=10}
            next_token();
            if (token != Num) {
                printf("%d: bad enum initializer\n", line_num);
                exit(-1);
            }
            i = token_val;
            next_token();
        }

        curr_id[Class] = Num;
        curr_id[Type] = INT;
        curr_id[Value] = i++;

        if (token == ',') {
            next_token();
        }
    }
}

void global_declaration() {
    // global_declaration ::= enum_decl | variable_decl | function_decl
    //
    // enum_decl ::= 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num'} '}'
    //
    // variable_decl ::= type {'*'} id { ',' {'*'} id } ';'
    //
    // function_decl ::= type {'*'} id '(' parameter_decl ')' '{' body_decl '}'


    int type; // tmp, actual type for variable
    int i; // tmp

    base_type = INT;

    // parse enum, this should be treated alone.
    if (token == Enum) {
        // enum [id] { a = 10, b = 20, ... }
        match(Enum);
        if (token != '{') {
            match(Id); // skip the [id] part
        }
        if (token == '{') {
            // parse the assign part
            match('{');
            enum_declaration();
            match('}');
        }

        match(';');
        return;
    }

    // parse type information
    if (token == Int) {
        match(Int);
    }
    else if (token == Char) {
        match(Char);
        base_type = CHAR;
    }

    // parse the comma seperated variable declaration.
    while (token != ';' && token != '}') {
        type = base_type;
        // parse pointer type, note that there may exist `int ****x;`
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }

        if (token != Id) {
            // invalid declaration
            printf("%d: bad global declaration\n", line_num);
            exit(-1);
        }
        if (curr_id[Class]) {
            // identifier exists
            printf("%d: duplicate global declaration\n", line_num);
            exit(-1);
        }
        match(Id);
        curr_id[Type] = type;

        if (token == '(') {
            curr_id[Class] = Fun;
            curr_id[Value] = (int)(text + 1); // the memory address of function
            function_declaration();
        } else {
            // variable declaration
            curr_id[Class] = Glo; // global variable
            curr_id[Value] = (int)data; // assign memory address
            data = data + sizeof(int);
        }

        if (token == ',') {
            match(',');
        }
    }
    next_token();
}
*/

void enum_declaration(){
    int i=0;
    while(token !='{'){  
        if(token != Id){
        printf("ERROR in line %d: invalid enum identefier\n",line_num);
        free_all();
        exit(-1);
        }
        next_token();
        if (token == Assign) {
            next_token();
            if (token != Num) {
                printf("ERROR in line %d: invalid enum initializer\n", line_num);
                free_all();
                exit(-1);
            }
            i = token_val;
            next_token();
        }

        curr_id[Class] = Num;
        curr_id[Type] = INT;
        curr_id[Value] = i++;

        if (token == ',') {
            next_token();
        }

    }
}

void global_declaration(){
    // global_declaration ::= enum_decl | variable_decl | function_decl
    //
    // enum_decl ::= 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num'} '}'
    //
    // variable_decl ::= type {'*'} id { ',' {'*'} id } ';'
    //
    // function_decl ::= type {'*'} id '(' parameter_decl ')' '{' body_decl '}'

    int type =0;
    int i=0;
    base_type =INT;
    
    //parse enum
    if(token == Enum){
        match(Enum);
        if(token !='{'){  //their is a name
            match(Id);
        }
        if(token == '{'){
            match('{');
            enum_declaration();
            match('}');
        }
        match(';');
        return;
    }
    //parse int
    if (token == Int) {
        match(Int);
    } 
    else if (token == Char) {
        match(Char);
        base_type = CHAR;
    }

    while(token  != ';' && token != '}'){
        type = base_type;
        while(token ==Mul){ // handle int**
            match(Mul);
            type = type +PTR;
        }
        if(token != Id){
            printf("ERROR in line %d: invalid global declaration\n",line_num);
            free_all();
            exit(-1);
        }
        if(curr_id[Class]){
            printf("ERROR in line %d: duplicate declaration\n",line_num);
            free_all();
            exit(-1);
        }
        match(Id);
        curr_id[Type] = type;

        if (token == '(') {
            curr_id[Class] = Fun;
            curr_id[Value] = (int)(text + 1); // memory address of function
            function_declaration();
        } else {
            // variable declaration
            curr_id[Class] = Glo; // global variable
            curr_id[Value] = (int)data; // assign memory address
            data = data + sizeof(int);
        }

        if (token == ',') {
            match(',');
        }
    }
    next_token();
    
}


void program() {
    // get next_token token
    next_token();
    while (token > 0) {
        global_declaration();
    }
}

/*
int eval() {
    int op, *tmp;
    while (1) {
        op = *pc++; // get next_token operation code

        if (op == IMM)       {ax = *pc++;}                                     // load immediate value to ax
        else if (op == LC)   {ax = *(char *)ax;}                               // load character to ax, address in ax
        else if (op == LI)   {ax = *(int *)ax;}                                // load integer to ax, address in ax
        else if (op == SC)   {ax = *(char *)*sp++ = ax;}                       // save character to address, value in ax, address on stack
        else if (op == SI)   {*(int *)*sp++ = ax;}                             // save integer to address, value in ax, address on stack
        else if (op == PUSH) {*--sp = ax;}                                     // push the value of ax onto the stack
        else if (op == JMP)  {pc = (int *)*pc;}                                // jump to the address
        else if (op == JZ)   {pc = ax ? pc + 1 : (int *)*pc;}                   // jump if ax is zero
        else if (op == JNZ)  {pc = ax ? (int *)*pc : pc + 1;}                   // jump if ax is not zero
        else if (op == CALL) {*--sp = (int)(pc+1); pc = (int *)*pc;}           // call subroutine
        //else if (op == RET)  {pc = (int *)*sp++;}                              // return from subroutine;
        else if (op == ENT)  {*--sp = (int)bp; bp = sp; sp = sp - *pc++;}      // make new stack frame
        else if (op == ADJ)  {sp = sp + *pc++;}                                // add esp, <size>
        else if (op == LEV)  {sp = bp; bp = (int *)*sp++; pc = (int *)*sp++;}  // restore call frame and PC
        else if (op == LEA)  {ax = (int)(bp + *pc++);}                         // load address for arguments.

        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;


        else if (op == EXIT) { printf("exit(%d)\n", *sp); return *sp;}
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
        else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
        else {
            printf("unknown instruction:%d\n", op);
            return -1;
        }
    }
    return 0;
}
*/

int eval(){
    int op;
    int* tmp;
    while(1){
        op = *(pc++);
        switch(op){
            case LEA: ax = (int)(bp + *pc++);break;

            case IMM: ax = *pc++; break;
            case JMP: pc = (int*) *pc;break;
            case CALL: // call a function
                *(--sp) = (int)(pc+1); 
                pc = (int *)*pc;
                break;
            case JZ: pc = (!ax) ? (int *)*pc: pc + 1; break;
            case JNZ:pc = ax ? (int *)*pc:pc + 1; break;
            
            case ENT:// push old BP, set BP to new frame base, reserve stack for new funct
                *(--sp) = (int)bp; 
                bp = sp; 
                sp = sp - *pc++;
                break;
            //adjust the stack (remove arguments from frame)
            case ADJ: sp = sp + *(pc++); break;
            case LEV: //restore old call frame
                sp = bp; 
                bp = (int *)*sp++; 
                pc = (int *)*sp++;
                break;
            case LI: ax = *(int*)ax; break; // load int ax = ram[ax]
            case LC: ax = *(char*)ax; break;
            case SI: *(int*)*sp++ = ax; break; // ram[sp++] = ax
            case SC: *(char*)*(sp++) = (char) ax; break;
            case PUSH: *(--sp) = ax; break;
            
            //bit operations
            case OR: ax = ax | *(sp++); break;
            case XOR: ax = ax ^ *(sp++); break;
            case AND:ax = ax & *(sp++); break;
            case EQ: ax = ax == *(sp++); break;
            case NE: ax = ax != *(sp++); break;
            case LT: ax = ax > *(sp++);break;
            case GT:ax = ax < *(sp++); break;
            case LE:ax = ax >= *(sp++); break;
            case GE:ax = ax <= *(sp++); break;
            case SHL:ax = *(sp++) << ax; break;
            case SHR:ax = *(sp++) >> ax; break;
            case ADD:ax = ax + *(sp++); break;
            case SUB: ax = *(sp++) - ax; break;
            case MUL: ax = *(sp++) * ax; break;
            case DIV: ax = *(sp++) / ax; break;
            case MOD: ax = *(sp++) % ax; break;
            
            case OPEN: ax = open((char *)sp[1], sp[0]); break;
            case READ: ax = read(sp[2], (char *)sp[1], *sp); break;
            case CLOS: ax = close(*sp);break;
            case PRTF:
               tmp = sp + pc[1]; 
               ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]);
               break;
            case MALC: ax = (int)malloc(*sp); break;
            // case FREE: free((void*)*sp); break;
            case MSET: ax = (int)memset((char *)sp[2], sp[1], *sp); break;
            case MCMP: ax = memcmp((char *)sp[2], (char *)sp[1], *sp);break;
            case EXIT: 
                printf("exit(%d)\n",*sp);
                return *sp;
                break;
            default:
                printf("Unknown instruction %d\n", op);
                return -1;
        }
    }
    return 0;
}

// #undef int // Mac/clang needs this to compile

int main(int argc, char **argv)
{
    #define int intptr_t

    int i, fd;
    int *tmp;

    argc--;
    argv++;

    poolsize = 256 * 1024; // arbitrary size
    line_num = 1;

    // allocate memory for virtual machine
    if (!(text = prev_text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }
    if (!(symbol_table = malloc(poolsize))) {
        printf("could not malloc(%d) for symbol table\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);
    memset(symbol_table, 0, poolsize);
    bp = sp = (int *)((int)stack + poolsize);
    ax = 0;

    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";

     // add keywords to symbol table
    i = Char;
    while (i <= While) {
        next_token();
        curr_id[Token] = i++;
    }

    // add library to symbol table
    i = OPEN;
    while (i <= EXIT) {
        next_token();
        curr_id[Class] = Sys;
        curr_id[Type] = INT;
        curr_id[Value] = i++;
    }

    next_token(); curr_id[Token] = Char; // handle void type
    next_token(); id_main = curr_id; // keep track of main


    // read the source file
    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    if (!(src = prev_src = malloc(poolsize))) {
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }
    // read the source file
    if ((i = read(fd, src, poolsize-1)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // add EOF character
    close(fd);

    program();

    if (!(pc = (int *)id_main[Value])) {
        printf("main() not defined\n");
        return -1;
    }

    // setup stack
    sp = (int *)((int)stack + poolsize);
    *--sp = EXIT; // call exit if main returns
    *--sp = PUSH; tmp = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)tmp;

    return eval();
}
