#include <stdio.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define PT 14
#define WIDTH_SCALE 1 
#define WIDTH   PT*WIDTH_SCALE
#define HEIGHT  PT 

typedef struct OBS_char {
    FT_ULong charcode;
    unsigned char* image;
    struct OBS_char* match;
} OBS_char;

void update_max_dim(FT_Bitmap* bitmap, 
        FT_Int x, FT_Int y, int* max_width, int* max_height){
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;
    for ( i = x, p = 0; i < x_max; i++, p++ ){
        for ( j = y, q = 0; j < y_max; j++, q++ ){
            if ( i < 0 || j < 0){
                continue;
            }
            if (bitmap->buffer[q * bitmap->width + p]){
                if (i > *max_width) { 
                    *max_width = i;
                }
                if (j > *max_height) {
                    *max_height = j;
                }
            }
        }
    }
}
void draw_bitmap(unsigned char* image, 
        FT_Bitmap* bitmap, FT_Int x, FT_Int y, int img_width, int img_height){
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;
    /* for simplicity, we assume that `bitmap->pixel_mode' */
    /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */
    for (i = x, p = 0; i < x_max; i++, p++){
        for (j = y, q = 0; j < y_max; j++, q++){
            if (i < 0 || j < 0 || i >= img_width || j >= img_height){
                continue;
            }
            image[j*img_width+i] |= bitmap->buffer[q * bitmap->width + p];
        }
    }
}

void show_image( unsigned char* image, int width, int height){
    int  i, j;
    for ( i = 0; i < height; i++ ){
        for ( j = 0; j < width; j++ ){
            unsigned char v = *(image++);
            if (v == 0) {
                putchar( ' ' );
            }
            else if (v < 128) {
                putchar( '+' );
            }
            else {
                putchar( '*' );
            }
        }
        putchar( '\n' );
    }
}

char s[5] = {0};

int len_UTF8(char c){
    return ((c&0b10000000) == 0)*1
         + ((c&0b11100000) == 0b11000000)*2 
         + ((c&0b11110000) == 0b11100000)*3 
         + ((c&0b11111000) == 0b11110000)*4; 
}
int len_charcode(unsigned long  charcode){
    return 1
         + (charcode >= 0x80) 
         + (charcode >= 0x800) 
         + (charcode >= 0x10000) 
         + (charcode >= 0x110000)*(-10);
}
char * charcode_to_UTF8(unsigned long charcode){
    int len = len_charcode(charcode);
    int mask = 0b11111111>>len;
    int first = len == 1 ? 0 : (0b111100000000>>len)&0b11111111;
    s[0] = ((charcode>>(6*(len-1)))&mask)|first;
    for (int i = len-1; i>0; i--){
        s[len-i] = ((charcode>>(6*(i-1)))&0b00111111)|0b10000000;
    }
    s[len] = '\0';
    return s;
}
unsigned long UTF8_to_charcode(char *cstring){
    unsigned long result = 0;
    int len = len_UTF8(*cstring);
    int mask = 0b11111111>>len;
    result = (*cstring)&mask;
    for (int i = 1; i< len; i++){
        result<<=6;
        result |= ((*(cstring+i))&0b00111111);
    }
    return result;
}

int img_diff_overlap(unsigned char* img1, unsigned char* img2, int width, int height){
    int sum = 0;
    int has_overlap = 0;
    int sum1 = 0;
    int sum2 = 0;
    for (int i = 0; i < width*height; i++){
        int v1 = *img1;
        int v2 = *img2;
        sum1 += v1;
        sum2 += v2;
        int overlap = v1>0 && v2>0;
        int one = v1>0 ^ v2>0;
        int abs_diff = v1>v2? v1-v2 : v2-v1;
        if (one){
            sum +=1;
        }
        if (overlap){
            sum -= 1; 
            has_overlap = 1;
        }
        img1++;
        img2++;
    }
    return sum;
}
int img_diff(unsigned char* img1, unsigned char* img2, int width, int height){
    return img_diff_overlap(img1, img2, width, height);
}

void print_overlap(unsigned char* img1, unsigned char* img2, int width, int height){
    int  i, j;
    for ( i = 0; i < height; i++ ){
        for ( j = 0; j < width; j++ ){
            int overlap = *(img1)>0 && *(img2)>0;
            int one = *(img1)>0 ^ *(img2)>0;
            if (overlap) {
                putchar( 'O' );
            }
            else if (one) {
                putchar( '!' );
            }
            else {
                putchar( ' ' );
            }
            img1++;
            img2++;
        }
        putchar( '\n' );
    }
}

int is_control(unsigned long charcode){
    //ranges from here:
    //https://www.aivosto.com/articles/control-characters.html
    int c0 = charcode <= 0x20 || charcode == 0x7F;
    int c1 = 0x80 <= charcode && charcode <= 0x9F;
    int ISO8859 = charcode == 0xA0 || charcode == 0xAD;
    return c0 || c1 || ISO8859;
}

int count_chars(FT_Face face, int* max_width, 
        int* max_height, int* control_cnt, int* max_charcode){
    FT_Error error;
    int cnt = 0;
    *control_cnt = 0;
    *max_charcode = 0;
    FT_ULong charcode;
    FT_UInt gid;
    charcode = FT_Get_First_Char(face, &gid);
    while (gid != 0)
    {
        if (*max_charcode < charcode){
            *max_charcode = charcode;
        }
        while (is_control(charcode)) {
            (*control_cnt)++;
            charcode = FT_Get_Next_Char(face, charcode, &gid);
            if (*max_charcode < charcode){
                *max_charcode = charcode;
            }
            if (gid == 0) {
                break;
            }
        }
        if (gid == 0) {
            break;
        }
        error = FT_Load_Glyph(face, gid, FT_LOAD_RENDER);
        if (error){
            fprintf(stdout, "error: FT_Load_Glyph {%x}\n", error);
        } else {
            update_max_dim(&face->glyph->bitmap, 
                    face->glyph->bitmap_left, 0, max_width, max_height);
        }
        charcode = FT_Get_Next_Char(face, charcode, &gid);
        cnt ++;
    } 
    return cnt;
}

int count_UTF8(char* text, 
        OBS_char ** charcode_to_character, int * unknown_character_len){
    int len = 0;
    *unknown_character_len = 0;
    while (*text){
        FT_ULong charcode = UTF8_to_charcode(text);
        len++;
        if(!charcode_to_character[charcode]){
            (*unknown_character_len)++;
        }
        text += len_UTF8(*text);
    }
    return len;
}

void text_to_OBS(char* text, OBS_char** charcode_to_character, OBS_char** OBS, FT_ULong max_charcode){
    while(*text){
        FT_ULong charcode = UTF8_to_charcode(text);
        if (charcode <= max_charcode){
            *(OBS++) = charcode_to_character[charcode];
        } else {
            *(OBS++) = 0;
        }
        text += len_UTF8(*text);
    }
}

int OBS_to_UTF8_len(OBS_char** characters, int len){
    int text_len = 0;
    for (int i = 0; i<len; i++){
        if (characters[i]){
            text_len += len_charcode(characters[i]->charcode);
        }
    }
    return text_len;
}

int OBS_to_UTF8_match_len(OBS_char** characters, int len){
    int text_len = 0;
    for (int i = 0; i<len; i++){
        if (characters[i] && characters[i]->match){
            text_len += len_charcode(characters[i]->match->charcode);
        }
    }
    return text_len;
}

void OBS_to_text(OBS_char** characters, int text_len, char* text){
    char * ch = text;
    while (ch<(text+text_len)){
        if (*characters){
            char * uni = charcode_to_UTF8((*characters)->charcode);
            while (*uni){
                *ch++ = *uni++;
            }
        }
        characters++;
    }
    *ch = '\0';
}

void OBS_to_match_text(OBS_char** characters, int text_len, char* text){
    char * ch = text;
    while (ch<(text+text_len)){
        if (*characters && (*characters)->match){
            char * uni = charcode_to_UTF8((*characters)->match->charcode);
            while (*uni){
                *ch++ = *uni++;
            }
        }
        characters++;
    }
    *ch = '\0';
}

int main( int argc, char**  argv ){
    if ( argc != 4 ){
        fprintf(stderr, "usage: %s font [t|f] text\n", argv[0]);
        exit(1);
    }

    char * input_font_filename = argv[1];                         
    char t_or_f = argv[2][0];
    char* text;
    if (t_or_f == 't'){
        text = argv[3];                         
    } else if (t_or_f == 'f') {
        long len;
        FILE* f = fopen(argv[3], "rb");
        if (f){
            fseek(f, 0, SEEK_END);
            len = ftell(f);
            fseek (f, 0, SEEK_SET);
            text = calloc(len+1,sizeof(char));
            if (text){
                fread(text, 1, len, f);
                text[len] = '\0';
            } 
            else {
                fprintf(stderr, "Failed to calloc\n");
            }
            fclose (f);
            fprintf(stdout, text);
        }
        else {
            fprintf(stdout, "could not open file: {%s}", argv[3]);
        }
    } else {
        fprintf(stderr, "usage: %s font [t|f] text\n", argv[0]);
        exit(1);
    }

    FT_Library library;
    FT_Error error;
    error = FT_Init_FreeType(&library);
    if (error){
        fprintf(stdout, "error: FT_Init_FreeType {%x}\n", error);
    } 

    FT_Face face;
    error = FT_New_Face( library, input_font_filename, 0, &face );
    if (error){
        fprintf(stdout, "error: FT_New_Face {%x}\n", error);
    } 
    error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    if (error){
        fprintf(stdout, "error: FT_Select_Charmap {%x}\n", error);
    } 
    error = FT_Set_Char_Size( face,  WIDTH * 64, HEIGHT * 64, 100, 0 ); 
    if (error){
        fprintf(stdout, "error: FT_Set_Char_Size {%x}\n", error);
    } 

    FT_Matrix matrix;
    double angle = 0;
    matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
    matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
    matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
    matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

    FT_Vector pen;   
    pen.x = 0;
    pen.y = 0;
    FT_Set_Transform( face, &matrix, &pen );

    int img_width = 0;
    int img_height = 0;
    int control_cnt, max_charcode;
    int non_control_cnt = 
        count_chars(face, &img_width, &img_height, &control_cnt, &max_charcode);

    fprintf(stdout, "non control glyphs: {%d}\n", non_control_cnt);
    fprintf(stdout, "control glyphs: {%d}\n", control_cnt);
    fprintf(stdout, "img width: {%d}\n", img_width);
    fprintf(stdout, "img height: {%d}\n", img_height);
    fprintf(stdout, "max charcode: {%d}\n", max_charcode);
    
    int to_alloc = non_control_cnt*img_width*img_height;
    unsigned char* images = calloc(to_alloc,sizeof(unsigned char));
    if (images == NULL){
        fprintf(stderr, "calloc fault, images");
    }
    OBS_char* characters = calloc((non_control_cnt+control_cnt),sizeof(OBS_char));
    if (characters == NULL){
        fprintf(stderr, "calloc fault, characters");
    }
    OBS_char** charcode_to_character = calloc((max_charcode+1),sizeof(OBS_char*));
    if (charcode_to_character == NULL){
        fprintf(stderr, "calloc fault, charcode_to_character");
    }

    FT_UInt gid;
    FT_ULong charcode = FT_Get_First_Char(face, &gid);
    unsigned char* img = images;
    OBS_char* ch = characters;
    while (gid != 0){
        while (is_control(charcode)) {
            ch->charcode = charcode;
            ch->image = 0;
            ch->match = ch;
            charcode_to_character[charcode] = ch;
            ch++;
            charcode = FT_Get_Next_Char(face, charcode, &gid);
            if (gid == 0) {
                break;
            }
        }
        if (gid == 0) {
            break;
        }
        ch->charcode = charcode;
        error = FT_Load_Glyph(face, gid, FT_LOAD_RENDER);
        if (error){
            fprintf(stdout, "error: FT_Load_Glyph {%x}\n", error);
        } else {
            draw_bitmap(img, &face->glyph->bitmap, 
                    face->glyph->bitmap_left, 0, img_width, img_height );
            ch->image = img;
            ch->match = 0;
            img += img_width*img_height;
        }
        charcode_to_character[charcode] = ch;
        ch++;
        charcode = FT_Get_Next_Char(face, charcode, &gid);
    } 

    int unknown_character_len;
    int input_len = count_UTF8(text, charcode_to_character, &unknown_character_len);
    fprintf(stdout, "unknown_character_len : {%d}\n", unknown_character_len);
    OBS_char ** input_characters = 
        calloc((input_len),sizeof(OBS_char*));

    text_to_OBS(text, charcode_to_character, input_characters, max_charcode);

    for (int i = 0; i< input_len; i++){
        OBS_char* ch1 = input_characters[i];
        if (ch1 == NULL){
            continue;
        }
        if (ch1->match != 0) {
            continue;
        }
        fprintf(stdout, "%06d: U+%04x\n",i,ch1->charcode);
        int min_diff = 1000000;
        OBS_char* min_char = 0;
        OBS_char* ch2 = characters;
        while (ch2 < (characters+non_control_cnt)){
            if (is_control(ch2->charcode) || ch1->charcode == ch2->charcode){
                ch2++;
                continue;
            }
            int diff = img_diff(ch1->image,ch2->image, img_width, img_width);
            if (diff<min_diff){
                min_diff = diff;
                min_char = ch2;
            }
            ch2++;
        }
        ch1->match = min_char;
    }

    for (int i = 0; i< input_len; i++){
        OBS_char* ch = input_characters[i];
        if (!ch) {
            continue;
        }
        fprintf(stdout, "diff : {U+%04x} {U+%04x}\n", 
                ch->charcode, ch->match->charcode);
        if (is_control(ch->charcode) || is_control(ch->match->charcode)){
            continue;
        }
        print_overlap(ch->image,ch->match->image, img_width, img_height);
    }
    fprintf(stdout,"\n");

    for (int i = 0; i< input_len; i++){
        OBS_char* ch = input_characters[i];
        if (!ch) {
            continue;
        }
        fprintf(stdout,"%s {U+%04x} => ", 
                charcode_to_UTF8(ch->charcode), ch->charcode);
        fprintf(stdout,"%s {U+%04x}\n", 
                charcode_to_UTF8(ch->match->charcode), ch->match->charcode);
    }
    fprintf(stdout,"\n");
    int match_text_len = OBS_to_UTF8_match_len(input_characters, input_len);
    char* match_text = calloc(match_text_len,sizeof(char)); 
    if (!match_text){
        fprintf(stdout, "calloc error, match_text");
    }
    OBS_to_match_text(input_characters, match_text_len, match_text);
    fprintf(stdout, "%s => %s\n", text, match_text);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}
