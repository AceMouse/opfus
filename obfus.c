#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define PT 14
#define WIDTH_SCALE 1 
#define WIDTH PT*WIDTH_SCALE
#define HEIGHT PT 


typedef struct OBS_match {
    FT_ULong charcode;
    int score;
} OBS_match;

typedef struct OBS_char {
    FT_ULong charcode;
    unsigned char* image;
    int match_count;
    struct OBS_match* matches;
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

int is_empty(unsigned char* image, int width, int height){
    int i;
    for (i = 0; i < height*width; i++){
        if (*(image++) != 0) {
            return 0;
        }
    }
    return 1;
}
void eraze_image(unsigned char* image, int width, int height){
    int i;
    for (i = 0; i < height*width; i++){
        *(image++) = 0;
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
int len_charcode(uint32_t charcode){
    return 1
         + (charcode >= 0x80) 
         + (charcode >= 0x800) 
         + (charcode >= 0x10000) 
         + (charcode >= 0x110000)*(-10);
}
char * charcode_to_UTF8(uint32_t charcode){
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
uint32_t UTF8_to_charcode(char *cstring){
    uint32_t result = 0;
    int len = len_UTF8(*cstring);
    int mask = 0b11111111>>len;
    result = (*cstring)&mask;
    for (int i = 1; i< len; i++){
        result<<=6;
        result |= ((*(cstring+i))&0b00111111);
    }
    return result;
}

int is_control(uint32_t charcode){
    //ranges from here:
    //https://www.aivosto.com/articles/control-characters.html
    int c0 = charcode <= 0x20 || charcode == 0x7F;
    int c1 = 0x80 <= charcode && charcode <= 0x9F;
    int ISO8859 = charcode == 0xA0 || charcode == 0xAD;
    return c0 || c1 || ISO8859;
}

int count_UTF8(char* text){
    int len = 0;
    while (*text){
        len++;
        text += len_UTF8(*text);
    }
    return len;
}

uint32_t max_UTF8(char* text){
    uint32_t max = 0;
    while(*text){
        int len = len_UTF8(*text);
        uint32_t tmp = UTF8_to_charcode(text);
        if (tmp>max){
            max = tmp;
        }
        text += len;
    }
    return max;
}

uint32_t* text_to_unique_charcodes(char *text, int* len){
    uint32_t max = max_UTF8(text);
    uint8_t* set = calloc(max+1, sizeof(uint8_t));
    *len = 0; 
    while (*text){
        int l = len_UTF8(*text);
        uint32_t charcode = UTF8_to_charcode(text);
        *len += (set[charcode]&1)^1;
        set[charcode] |= 1;
        text += l;
    }
    uint32_t* charcodes = malloc(*len*sizeof(uint32_t));
    uint32_t* ch = charcodes;
    for (uint32_t i = 0; i<max+1; i++){
        if (set[i]){
            *(ch++) = i; 
        }
    }
    free(set);
    return charcodes;
}

int text_UTF8_len(char* text){
    int utf_len = 0;
    while(*text){
        int len = len_UTF8(*text);
        text += len;
        utf_len++;
    }
    return utf_len;
}

int img_diff_overlap(unsigned char* img1, unsigned char* img2, int width, int height){
    int sum = 0;
    int sum1 = 0;
    int sum2 = 0;
    for (int i = 0; i < width*height; i++){
        int v1 = *img1;
        int v2 = *img2;
        sum1 += v1;
        sum2 += v2;
        int overlap = (v1>0) && (v2>0);
        int one = (v1>0) ^ (v2>0);
        if (one){
            sum +=1;
        }
        if (overlap){
            sum -= 1; 
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
            int overlap = (*(img1)>0) && (*(img2)>0);
            int one = (*(img1)>0) ^ (*(img2)>0);
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


int match_compare(const void *a, const void *b) {
    return (((OBS_match *)a)->score - ((OBS_match *)b)->score);
}



char* obscure(char* text, OBS_char* unique_input_OBS, 
        int unique_count, int32_t max_input_charcode, int difficulty){
    int max_out_len = text_UTF8_len(text)*4;
    char* out = calloc(max_out_len+1, sizeof(char));
    char* o = out;
    while(*text){
        int next_len = len_UTF8(*text);
        int32_t charcode = UTF8_to_charcode(text);
        int idx = 0;
        for(int i = 0; i<unique_count; i++){
            if (unique_input_OBS[i].charcode == charcode){
                idx = i;
                break;
            }
        }
        uint32_t match = charcode;
        if (unique_input_OBS[idx].matches){
            int worst_allowed_index = unique_input_OBS[idx].match_count*difficulty/100;
            int best_allowed_index = 1;
            if (best_allowed_index <= worst_allowed_index){
                int index = (rand()%(worst_allowed_index+1-best_allowed_index))+best_allowed_index;
                match = unique_input_OBS[idx].matches[index].charcode;
            }
        }
        char* s = charcode_to_UTF8(match);
        while(*s){
            *(o++) = *(s++);
        }
        text += next_len;
    }
    return out;
}

int main(int argc, char** argv){
    if (argc != 4){
        fprintf(stderr, "usage: %s font t text\n"
                        "       %s font f file-path\n", argv[0], argv[0]);
        exit(1);
    }

    char * actual_font_filename = argv[1];                         
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
    srand(time(0));
    FT_Library library;
    FT_Error error;
    error = FT_Init_FreeType(&library);
    if (error){
        fprintf(stdout, "error: FT_Init_FreeType {%x}\n", error);
    } 

    FT_Face actual_face;
    error = FT_New_Face(library, actual_font_filename, 0, &actual_face );
    if (error){
        fprintf(stdout, "error: FT_New_Face {%x}\n", error);
    } 
    FT_Face mimic_face = actual_face;
    error = FT_Select_Charmap(actual_face, FT_ENCODING_UNICODE);
    if (error){
        fprintf(stdout, "error: FT_Select_Charmap {%x}\n", error);
    } 
    error = FT_Set_Char_Size(actual_face,  WIDTH * 64, HEIGHT * 64, 100, 0); 
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
    FT_Set_Transform(mimic_face, &matrix, &pen);
    int img_width = 0;
    int img_height = 0;
    int max_input_charcode = max_UTF8(text);
    int max_charcode = max_input_charcode;
    int control_cnt;
    int non_control_cnt = 
        count_chars(actual_face, &img_width, &img_height, &control_cnt, &max_charcode);

    /*fprintf(stdout, "max charcode in input: {%d}\n", max_input_charcode);
    fprintf(stdout, "max charcode in font & input: {%d}\n", max_charcode);
    fprintf(stdout, "non control glyphs: {%d}\n", non_control_cnt);
    fprintf(stdout, "control glyphs: {%d}\n", control_cnt);
    fprintf(stdout, "img width: {%d}\n", img_width);
    fprintf(stdout, "img height: {%d}\n", img_height);*/ 

    int unique_count;
    uint32_t* unique_input_charcodes = text_to_unique_charcodes(text, &unique_count);
    OBS_char* unique_input_OBS = calloc(unique_count, sizeof(OBS_char));
    OBS_char* obs = unique_input_OBS;

    int to_alloc = (unique_count+1)*img_width*img_height;
    unsigned char* images = calloc(to_alloc,sizeof(unsigned char));
    unsigned char* img = images;

    for(int i = 0; i < unique_count; i++){
        //fprintf(stdout, "%d\n", i);
        obs->charcode = unique_input_charcodes[i];
        error = FT_Load_Char(mimic_face, unique_input_charcodes[i], FT_LOAD_RENDER);
        if (error){
            fprintf(stdout, "error: FT_Load_Glyph {%x} could not load glyph {U+%04lx}\n", error, obs->charcode);
            obs++;
            continue;
        }
        if (mimic_face->glyph->bitmap.rows != 0){
            obs->image = img;
            draw_bitmap(img, &mimic_face->glyph->bitmap, 
                        mimic_face->glyph->bitmap_left, 0, img_width, img_height);
            obs->matches = calloc((non_control_cnt+control_cnt),sizeof(OBS_match));
            obs->matches += non_control_cnt+control_cnt-1; // grow backwards;
            img += img_width*img_height;
        }
        obs++;
    }

    FT_UInt gid;
    FT_ULong charcode = FT_Get_First_Char(actual_face, &gid);
    while (gid != 0){
        //fprintf(stdout, "{U+%04x}\n", charcode);
        error = FT_Load_Glyph(actual_face, gid, FT_LOAD_RENDER);
        if (error){
            fprintf(stdout, "error: FT_Load_Glyph {%x}\n", error);
            charcode = FT_Get_Next_Char(actual_face, charcode, &gid);
            continue;
        }

        if (actual_face->glyph->bitmap.rows == 0){
            charcode = FT_Get_Next_Char(actual_face, charcode, &gid);
            continue;
        }

        eraze_image(img, img_width, img_height);// reuse +1 slot in images 
        draw_bitmap(img, &actual_face->glyph->bitmap, 
                actual_face->glyph->bitmap_left, 0, img_width, img_height);

        int min_diff = 1000000;
        int min_i = 0;
        for(int i = 0; i < unique_count; i++){
            obs = &unique_input_OBS[i];
            if (obs->image == NULL || obs->matches == NULL){
                continue;
            }
            int diff = img_diff(obs->image, img, img_width, img_width);
            if (diff<min_diff){
                min_diff = diff;
                min_i = i;
            }
        }
        unique_input_OBS[min_i].matches->charcode = charcode;
        unique_input_OBS[min_i].matches->score = min_diff;
        unique_input_OBS[min_i].match_count++;
        unique_input_OBS[min_i].matches--;

        charcode = FT_Get_Next_Char(actual_face, charcode, &gid);
    }

    for(int i = 0; i < unique_count; i++){
        if (unique_input_OBS[i].matches == NULL){
            continue;
        }
        qsort(unique_input_OBS[i].matches, unique_input_OBS[i].match_count,
                sizeof(OBS_match), match_compare);
    }
    fprintf(stdout, "%s ->\n", text);
    for(int i = 1; i < 100; i*=2){
        char* obscured = obscure(text, unique_input_OBS, unique_count, max_input_charcode, i);
        fprintf(stdout, "%s\n", obscured);
    }
    return 0;
}
