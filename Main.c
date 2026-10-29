#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define WORDLIST "WordList.txt"

bool Debug = false;

struct ListOfWords {
    char** words;
    int8_t* value;
    uint32_t length;
};

struct SingleCharacter {
    char character;
    uint8_t position;
};

uint16_t EncodeThreePartLengths(uint8_t GreenLen, uint8_t YellowLen, uint8_t GrayLen){
    uint16_t ReturnValue = 0;

    ReturnValue += GreenLen;
    ReturnValue = ReturnValue << 3;

    ReturnValue += YellowLen;
    ReturnValue = ReturnValue << 10;

    ReturnValue += GrayLen;

    return ReturnValue;
}

void PrintWordList(struct ListOfWords ListOfWords){
    printf("-----------------------------------------\n");
    for (int i = 0; i < ListOfWords.length; i++){
        printf("Word: %s   | value: %d\n",ListOfWords.words[i],ListOfWords.value[i]);
    }
    printf("-----------------------------------------\n");
}

void* ReallocateWordList(struct ListOfWords* ListOfWords, char* word, int8_t value){
    ListOfWords->length++;
    uint32_t length = ListOfWords->length;

    char** TempList = realloc(ListOfWords->words, sizeof(char* ) * length);
    if (TempList != NULL){
        ListOfWords-> words = TempList;
    }
    else{printf("realloc failed, PANICKING!\n"); exit(EXIT_FAILURE);}
    
    ListOfWords->words[length - 1] = malloc(strlen(word) + 1);
    strcpy(ListOfWords->words[length - 1], word);

    ListOfWords->value = realloc(ListOfWords->value, length);
    ListOfWords->value[length - 1] = value; 

    if (Debug == true){printf("re-allocation sucess\n");}
    return ListOfWords;
}

struct ListOfWords* GetWordsWithChar(struct ListOfWords* ListOfWords,char* include, char* exclude){
    FILE* fptr = fopen(WORDLIST,"r");
    char word[100] = { 0 };

    struct ListOfWords TempList;
    TempList.words = malloc(sizeof(char* ) * 1);
    TempList.value = malloc(sizeof(uint8_t ) * 1);
    TempList.length = 0;

    int8_t WorstValue = -127;
    int8_t TempValue = 0;

    while(1){
        if (fgets(word,sizeof(word),fptr) == NULL){break;}
        word[5] = '\0';
        
        TempValue = 0;
        for(int i = 0; i < strlen(include); i++){
            uint8_t wc = 0;

            while(wc < 5){
                if (word[wc] == *(include + i)){
                    TempValue++;
                }
                wc++;
            }    
        }

        for (int i = 0; i < strlen(exclude); i++){
            uint8_t wc = 0;

            while(wc < 5){
                if (word[wc] == *(exclude + i)){
                    TempValue--;
                }
                wc++;
            }
        }

        if (TempValue >= WorstValue){
            if (Debug == true){
                printf("word: %s with value: %d is good\n",word,TempValue);
            }

            WorstValue = TempValue;
            TempList = *(struct ListOfWords* )ReallocateWordList(&TempList, word, TempValue);
            continue;
        }
        else{
            continue;
        }
    }

    fclose(fptr);

    ListOfWords = &TempList;
    return ListOfWords;
}

struct ListOfWords* GetPossibleWords(struct ListOfWords* wordlist,struct SingleCharacter* GreenList, struct SingleCharacter* YellowList, struct SingleCharacter* GrayList,uint16_t lengths){
    FILE* fptr = fopen(WORDLIST,"r");

    //Decoding lengths, bit 1 - 3 = Len1, bit 4 - 6 = Len2, rest of bits is gray
    uint8_t GreenLength = (lengths & 57344) >> 13;
    uint8_t YellowLength = (lengths & 7168) >> 10;
    uint8_t GrayLength = (lengths & 1023);

    printf("Lengths: Green(%d), Yellow(%d), Gray(%d)\n",GreenLength,YellowLength,GrayLength);
    
    char word[100] = { 0 };

    struct ListOfWords TempList;
    TempList.words = malloc(sizeof(char* ) * 1);
    TempList.value = malloc(sizeof(int8_t ) * 1);
    TempList.length = 0;

    while(1){
        if (fgets(word,sizeof(word),fptr) == NULL){break;}
        word[5] = '\0';
        //printf("word: %s\n",word);

        //Iterate over the word to check if all the greens are in place
        for (uint8_t i = 0; i < GreenLength; i++){
            uint8_t position = (GreenList + i)->position;

            //printf("GreenChar: %c, Word char: %c\n",(GreenList + i)->character, word[position - 1]);

            if (word[position - 1] != (GreenList + i)->character){
                //Decoding the position into the value
                goto InvalidWord;
            }   
        }

        //Iterate over the word to make sure no gray characters are in place
        for (uint16_t i = 0; i < GrayLength; i++){
            for (uint8_t wc = 0; wc < 5; wc++){

                //printf("GrayChar: %c, Word char: %c\n",(GreenList + i)->character, word[wc]);

                if (word[wc] == (GrayList + i)->character){
                    goto InvalidWord;
                }
            }
        }

        

        for (int i = 0; i < YellowLength; i++){
            bool ContainsCharacter = false;

            for (uint8_t wc = 0; wc < 5; wc++){
             
                if (word[wc] == (YellowList + i)->character){
                    ContainsCharacter = true;
                    break;
                }
            }

            if (ContainsCharacter == false){goto InvalidWord;}
        }

        TempList = *(struct ListOfWords* )ReallocateWordList(&TempList, word, 0);
        //PrintWordList(TempList);
        if (Debug == true){
            printf("Word %d = %s passed\n",TempList.length ,TempList.words[TempList.length - 1]);
        }
        
        InvalidWord:
        continue;
    }

    wordlist = &TempList;

    fclose(fptr);
    return wordlist;
}

uint32_t EncodeCharacterList(struct SingleCharacter* List, uint8_t length){

    uint32_t ReturnValue = 0;

    for (int i = 0; i < length; i++){
        ReturnValue = ReturnValue | (uint8_t)1 << ((List + i)->character - 97);
    }

    if (Debug == true){
        printf("Encoded list: %u\n",ReturnValue);
    } 
    return ReturnValue;
}

char* GetPossibleCharacters (char* CharacterList, struct ListOfWords* wordlist,uint32_t GreenList, uint32_t YellowList){
    free(CharacterList);
    
    uint32_t UsedChars = 0;
    uint8_t UsedCharsLength = 0;

    for (int i = 0; i < wordlist->length; i++){
        char* ptr = wordlist->words[i];

        while(*ptr != '\0'){

            //Checks if character already in UsedChars
            if ((UsedChars & 1 << (*ptr - 97)) != 0){
                goto FailedClear;
            }

            //If ptr character is in GreenList and not in YellowList and there isn't repeat character in any word, continue since it only appears once.
            if ((GreenList & 1 << (*ptr - 97)) == 0){ //Greenlist check
                goto ClearedChecks;
            }

            if ((YellowList & 1 << (*ptr - 97)) != 0){ //Yellowlist check
                goto ClearedChecks;
            }

            char* ptr2 = ptr;
            ptr2++;
            while (*ptr2 != '\0') {
                if (*ptr2 == *ptr){
                    goto ClearedChecks;
                }
                ptr2++;
            }
            
            goto FailedClear;

            ClearedChecks:
            if (Debug == true){
                printf("Character %c was cleared\n",*ptr);
            }
            //Saving used characters in bit format bit 0 (1) = a
            UsedChars = UsedChars | (uint32_t)1 << (*ptr - 97); 
            UsedCharsLength++;

            FailedClear:
            ptr++;
        }
    }

    CharacterList = malloc(UsedCharsLength + 1);
    CharacterList[UsedCharsLength] = '\0';

    UsedCharsLength = 0;
    for (uint8_t i = 0; i < 32; i++){
        if ((UsedChars & 1 << i) != 0){
            CharacterList[UsedCharsLength] = i + 97;
            UsedCharsLength++;

        }
    }

    if (Debug == true){
        printf("possible characters are: %s\n",CharacterList);
    }

    return CharacterList;
}

struct SingleCharacter* ConstructList(struct SingleCharacter* List, char* characters){
    char* ptr = characters;
    free(List);
    if (Debug == true){
        printf("managed to free List\n");
    }

    List = malloc(sizeof(struct SingleCharacter) * strlen(characters));

    uint8_t position = 0; 
    while(*ptr != '\0'){
        (List + position)->character = *ptr;
        (List + position)->position = 1;
        printf("char %d: %c\n",position, (List+position)->character);

        position++;
        ptr++;
    }

    return List;
}

struct SingleCharacter* ConstructListWithPositions(struct SingleCharacter* List, char* characters,char* positions){
    char* ptr = characters;
    free(List);
    if (Debug == true){
        printf("managed to free List\n");
    }

    List = malloc(sizeof(struct SingleCharacter) * strlen(characters));

    uint8_t position = 0; 
    while(*ptr != '\0'){
        (List + position)->character = *ptr;
        (List + position)->position = *(positions + position) - 48;
        printf("char %d: %c | position: %d\n",position, (List+position)->character, (List +position)->position);

        position++;
        ptr++;
    }

    return List;
}

char* ChangeListIntoString(char* string, struct SingleCharacter* List, uint8_t length){
    free(string);

    char* TempString = malloc(length + 1);
    TempString[length] = '\0';

    for (uint8_t i = 0; i < length; i++){
        TempString[i] = (List + i)->character;
    }

    string = TempString;
    return string;
}

void InputCurrentStatus(){

    char StringBuffer[27];
    char CharBuffer;
    
    char TotalPositionBuffer[27];
    char PositionBuffer;
    int c = 0;
    
    struct SingleCharacter* GreenList;
    uint8_t CurrentChar = 0;
    while(1){
        if (CurrentChar == 26){
            printf("You have already inputed 26 characters, a WORDLE board has 5 maximum characters, inputing anything above 5 is already worrying but for some reason you decided to input 26 characters, i'm putting a stop to the amount of green characters, strongly reconsider your input -_-\n");
        }
        printf("Input 0 anywhere to exit\n");

        printf("Please input green character: ");
        scanf("%c",&CharBuffer);
        while ((c = getchar()) != '\n' && c != EOF);

        if (CharBuffer == '0'){
            break;
        }
        StringBuffer[CurrentChar] = CharBuffer; 

        printf("Please input the characters position: ");
        scanf("%c", &PositionBuffer);
        while ((c = getchar()) != '\n' && c != EOF);

        if (PositionBuffer == '0'){
            break;
        }
        TotalPositionBuffer[CurrentChar] = PositionBuffer;

        CurrentChar++;
    }

    GreenList = ConstructListWithPositions(GreenList, StringBuffer, TotalPositionBuffer);
    uint8_t GreenLength = strlen(StringBuffer);

    struct SingleCharacter* YellowList;
    printf("Please input all yellow characters in the same line (0 if there are none): ");
    scanf("%s",StringBuffer);
    if (StringBuffer[0] == '0'){StringBuffer[0] = '\0';}
    while ((c = getchar()) != '\n' && c != EOF);
    
    YellowList = ConstructList(YellowList, StringBuffer);
    uint8_t YellowLength = strlen(StringBuffer);

    struct SingleCharacter* GrayList; 
    printf("Please input all gray characters in the same line (0 if there are none): ");
    scanf("%s",StringBuffer);
    if (StringBuffer[0] == '0'){StringBuffer[0] = '\0';}
    while ((c = getchar()) != '\n' && c != EOF);

    GrayList = ConstructList(GrayList, StringBuffer);
    uint8_t GrayLength = strlen(StringBuffer);

    struct ListOfWords words;
    words = *GetPossibleWords(&words, GreenList, YellowList, GrayList, EncodeThreePartLengths(GreenLength, YellowLength, GrayLength));

    printf("Possible words that match all criteria are:\n");
    PrintWordList(words);

    printf("Input anything to continue..");
    scanf("%c",&CharBuffer);
    while ((c = getchar()) != '\n' && c != EOF);

    char* CharacterList = malloc(1);
    CharacterList = GetPossibleCharacters(CharacterList, &words, EncodeCharacterList(GreenList, GreenLength), EncodeCharacterList(YellowList, YellowLength));

    printf("Missing characters that might be in the last words: %s\n",CharacterList);
    printf("Input anything to continue..");
    scanf("%c",&CharBuffer);
    while ((c = getchar()) != '\n' && c != EOF);

    char* GrayString = malloc(1);
    words = *GetWordsWithChar(&words, CharacterList,ChangeListIntoString(GrayString, GrayList, GrayLength));
    printf("Best possible guesses in order:\n");
    PrintWordList(words);
}

int main(int argc, char** argv){

    //GetWordsWithChar("afeoc", "kmnzqwpl");

    InputCurrentStatus();

    return 0;
}