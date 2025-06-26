// doesnt soport macros, struct, /* */,


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 


#define int int64_t
#define POOL_SIZE 256*1024

void free_all();
int token;
int token_val; 

char* src = NULL;
char* prev_src = NULL;
int pool_size;
int line_num;
int *curr_id = NULL;
int *symbol_table = NULL;

//for the vm
//memory segments
int *text = NULL; 
int *prev_text = NULL;
int *stack = NULL;
char* data = NULL;
//regs
int* pc;
int*sp;
int* bp; //base pointer
int ax;
int* cycle;

int* id_main =NULL;
int base_type;  
int expr_type;

int index_of_bp; // index of bp pointer on stack

// vm instructions
enum { LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT };

// tokens (operators last and in precedence order)
enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// fields of identifier
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};

enum{ CHAR, INT,PTR};


void next_token(){
    // token = *(src++);
    char *last_pos;
    int hash;
    while(token = *src){ // while will skip unknown characters
        src++;
        //identifer and symbol table (uniqe id for var name etc)
        if((token >='a' &&token <='z')||(token >='A' &&token <='Z')||token =='_' ){
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
        else if(token >='0' || token<='9'){
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
            case '\n': 
                line_num++;
                break;
            case '#': // skip macros
                while(*src != 0 &&*src != '\n') src++;
                break;
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
            case '%': token = Xor; return;
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
            default:
                break;
            }
        }
        
    }
}

void match(int tk){
    if (token == tk){
        next_token();
    }
    else {
        printf("ERROR in line %d: expected the token:%d (got %d)\n",line_num,tk,token);
        free_all();
        exit(-1);
    }
}
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
    base_type =Int;
    
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
            printf("ERROR in line %d: invalid declaration\n",line_num);
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
void program(){
    next_token();
    while(token >0){
        //printf("token = %c\n",token);
        // next_token();
        global_declaration();

    }
}
int eval(){
    int op;
    int* tmp;
    while(1){
        op = *(pc++);
        switch(op){
            case LEA:
            break;

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
            case FREE: free((void*)*sp); break;
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
void free_all(){
    free(prev_src);
    free(prev_text);
    free(data);
    free(stack);
}
/*
================================== MAIN ==================================
*/
int main(int argc, char** argv) {
    // for(int i=0;i<argc;i++){
    //     printf("%s\n",argv[i]);
    // }
    argc--;
    argv++;

   // int fd;
    int i;
    // pool_size = POOL_SIZE;
    line_num = 1;
    // FILE* fp = fopen(*argv,"rb");
    int fd = open(*argv,0);
    if(fd<0){
        printf("ERROR: coud not open the file (%s)\n",*argv);
        return -1;
    }

    //mem for the vm
    text = malloc(POOL_SIZE);
    if(!text){
        printf("ERROR: malloc for text segment\n");
        //todo free all 
        return -1;
    }
    memset(text,0,POOL_SIZE);
    prev_text = text;
    data = malloc(POOL_SIZE);
    if(!data){
        printf("ERROR: malloc for data segment\n");
        //todo free all 
        return -1;
    }
    memset(data,0,POOL_SIZE);
    stack = malloc(POOL_SIZE);
    if(!stack){
        printf("ERROR: malloc for stack segment\n");
        //todo free all 
        return -1;
    }
    memset(stack, 0, POOL_SIZE);
        symbol_table = malloc(POOL_SIZE);
    if(!symbol_table){
        printf("ERROR: malloc for symbol_table\n");
    }

    sp = (int*)((int)stack + POOL_SIZE);
    bp =sp;
    ax =0;

    src = "char else enum if int return sizeof while "
      "open read close printf malloc free memset memcmp exit void main";

    i = Char;
    while(i<=While){
        next_token();
        curr_id[Token] = i++;
    }
    i = OPEN;
    while(i<=EXIT){
        curr_id[Class] = Sys;
        curr_id[Type] = INT;
        curr_id[Value] = i++;
    }

    next_token();
    curr_id[Token] = Char;
    next_token();
    id_main = curr_id;

    src = malloc(POOL_SIZE);
    if(!src){
        printf("ERROR: malloc for src code\n");
        close(fd);
        return -1;
    }
    prev_src =src;
    
    i = read(fd, src, POOL_SIZE-1); // fread(src, sizeof(char), POOL_SIZE - 1, fp);
     if (i<1){
        printf("ERROR: at read() return value: %d \n", i); 
        return -1;
    }

    // src[i]=0;
    // close(fd);

   

    //test 
    text[i++] =IMM;
    text[i++] = 10;
    text[i++] = PUSH;
    text[i++] = IMM;
    text[i++] = 20;
    text[i++] = ADD;
    text[i++] = PUSH;
    text[i++] = EXIT;
    pc = text;  


    program();

    int status = eval();
    free_all();
    return status;
}