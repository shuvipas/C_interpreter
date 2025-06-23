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

int token;
int token_val; 

char* src;
char* prev_src;
int pool_size;
int line_num;
int *curr_id;
int *symbol_table;

//for the vm
//memory segments
int *text; 
int *prev_text;
int *stack;
char* data;
//regs
int* pc;
int*sp;
int* bp; //base pointer
int ax;
int* cycle;

// vm instructions
enum { LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };

// tokens (operators last and in precedence order)
enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// fields of identifier
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};



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
            case '"':
            case '\'':

                break;
            
            
            default:
                break;
            }
        }
        
    }
}

void expression(int level) {
    // do nothing
}
void program(){
    next_token();
    while(token >0){
        //printf("token = %c\n",token);
        next_token();
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
    // int i;
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

    sp = (int*)((int)stack + POOL_SIZE);
    bp =sp;
    ax =0;




    src = malloc(POOL_SIZE);
    if(!src){
        printf("ERROR: coud not allocet size of %d\n",POOL_SIZE);
        close(fd);
        return -1;
    }
    prev_src =src;
    
    int i = read(fd, src, POOL_SIZE-1); // fread(src, sizeof(char), POOL_SIZE - 1, fp);
     if (i<1){
        printf("ERROR: at read() return value: %d \n", i); 
        free(src); 
        close(fd);
        return -1;
    }
    src[i]=0;
    close(fd);
    
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
    // free(prev_src);
    // free(prev_text);
    // free(data);
    // free(stack);
    return eval();
}