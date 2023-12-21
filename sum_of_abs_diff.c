/*
 * BUGS: 
 *      top of imgs cut off, move pen to midle!
 */
#include <stdio.h>
#include <math.h>
#include <stdint.h>

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


void
update_max_dim( FT_Bitmap* bitmap, FT_Int x, FT_Int y, int* max_width, int* max_height)
{
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;
    for ( i = x, p = 0; i < x_max; i++, p++ )
    {
        for ( j = y, q = 0; j < y_max; j++, q++ )
        {
            if ( i < 0      || j < 0){
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
void
draw_bitmap( unsigned char* image, FT_Bitmap* bitmap, FT_Int x, FT_Int y, int img_width, int img_height)
{
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;


    /* for simplicity, we assume that `bitmap->pixel_mode' */
    /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

    for ( i = x, p = 0; i < x_max; i++, p++ )
    {
        for ( j = y, q = 0; j < y_max; j++, q++ )
        {
            if ( i < 0      || j < 0       ||
                    i >= img_width || j >= img_height )
                continue;

            image[j*img_width+i] |= bitmap->buffer[q * bitmap->width + p];
        }
    }
}

void
show_image( unsigned char* image, int width, int height)
{
    int  i, j;
    for ( i = 0; i < height; i++ )
    {
        for ( j = 0; j < width; j++ ){
            unsigned char v = *(image++);
            putchar( v == 0 ? ' '
                    : v < 128 ? '+'
                    : '*' );
        }
        putchar( '\n' );
    }
}

void
eraze_image( unsigned char* image, int height, int width)
{
    int  i, j;


    for ( i = 0; i < height; i++ )
    {
        for ( j = 0; j < width; j++ ){
            *(image++) = 0;
        }
    }
}

char s[5] = {0};
char * to_unicode_string(FT_ULong charcode){
    if (charcode < 0x80){
        s[0] = charcode;
        s[1] = 0;
    }
    else if (charcode < 0x800){
        s[0] = 0b11000000|(charcode>>6)&0b11111;
        s[1] = 0b10000000|(charcode&0b111111);
        s[2] = 0;
    }    
    else if (charcode < 0x10000){
        s[0] = 0b11100000|(charcode>>12)&0b1111;
        s[1] = 0b10000000|((charcode>>6)&0b111111);
        s[2] = 0b10000000|(charcode&0b111111);
        s[3] = 0;
    }
    else if (charcode <= 0x10FFFF){
        s[0] = 0b11110000|(charcode>>18)&0b111;
        s[1] = 0b10000000|((charcode>>12)&0b111111);
        s[2] = 0b10000000|((charcode>>6)&0b111111);
        s[3] = 0b10000000|(charcode&0b111111);
        s[4] = 0;
    } 
    else {
        return 0;
    }
    return s;
}
int len_next_unicode(char *cstring){
    int four = (((*cstring)>>3)&0b11111) == 0b11110;
    int three = (((*cstring)>>4)&0b1111) == 0b1110;
    int two = (((*cstring)>>5)&0b111) == 0b110;
    int one = (((*cstring)>>7)&0b1) == 0;
    if (four){
        return 4;
    }
    if (three){
        return 3;
    }
    if (two){
        return 2;
    }
    if (one){
        return 1;
    }
    return 0;
}
FT_ULong unicode_string_to_charcode(char *cstring){
    FT_ULong result = 0;
    int four = (((*cstring)>>3)&0b11111) == 0b11110;
    int three = (((*cstring)>>4)&0b1111) == 0b1110;
    int two = (((*cstring)>>5)&0b111) == 0b110;
    int one = (((*cstring)>>7)&0b1) == 0;
    if (four){
        result = (((*cstring)&0b111) <<18) |  ((*(cstring+1)&0b111111)<<12) | ((*(cstring+2)&0b111111)<<6) | (*(cstring+3)&0b111111);
    }
    if (three){
        result = (((*cstring)&0b1111) <<12) |  ((*(cstring+1)&0b111111)<<6) | (*(cstring+2)&0b111111);
    }
    if (two){
        result = (((*cstring)&0b11111)<<6) | (*(cstring+1)&0b111111);
    }
    if (one){
        result = *cstring;
    }
    return result;
}

int img_sum_abs_diff(unsigned char* img1, unsigned char* img2, int width, int height){
    int sum = 0;
    for (int i = 0; i < width*height; i++){
        int v1 = *img1;
        int v2 = *img2;
        int abs_diff = v1>v2? v1-v2 : v2-v1;
        sum += abs_diff;
        img1++;
        img2++;
    }
    return sum;
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
        if (one){sum +=1/* v1+v2*/;}
        if (overlap){sum -= 1/*300/(1+abs_diff)*/; has_overlap = 1;}
        img1++;
        img2++;
    }
    //if (!has_overlap && sum1 != sum2){return 1000;}
    return sum;
}
int img_diff(unsigned char* img1, unsigned char* img2, int width, int height){
    return img_diff_overlap(img1,img2, width, height);
}

void print_overlap(unsigned char* img1, unsigned char* img2, int width, int height){
    int  i, j;
    for ( i = 0; i < height; i++ )
    {
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



int is_control(FT_UInt charcode){
    //ranges from here:
    //https://www.aivosto.com/articles/control-characters.html
    int c0 = charcode < 0x20 || charcode == 0x7F;
    int c1 = 0x80 <= charcode && charcode <= 0x9F;
    int ISO8859 = charcode == 0xA0 || charcode == 0xAD;
    return c0||c1||ISO8859;
}
int count_chars(FT_Face face, int* max_width, int* max_height, int* control_cnt, int* max_charcode){
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
            update_max_dim( &face->glyph->bitmap, face->glyph->bitmap_left, 0, max_width, max_height);
        }
        charcode = FT_Get_Next_Char(face, charcode, &gid);
        cnt ++;
    } 
    return cnt;
}
int
main( int argc, char**  argv )
{
    FT_Library    library;
    FT_Face       face;

    FT_GlyphSlot  slot;
    FT_Matrix     matrix;                 /* transformation matrix */
    FT_Vector     pen;                    /* untransformed origin  */
    FT_Error      error;

    char*         filename;
    char*          text;

    double        angle;
    int           target_height;
    int           n, num_chars;


    if ( argc != 3 )
    {
        fprintf ( stderr, "usage: %s font text\n", argv[0] );
        exit( 1 );
    }

    filename      = argv[1];                           /* first argument     */
    text          = argv[2];                           /* second argument    */
    uint8_t * b = text;
    while (*b){
        fprintf(stdout, "\\c{%b}",*(b++));
    }
    fprintf(stdout, " = \\u{%b} = %s\n", unicode_string_to_charcode(text), text);
    angle         = 0;// ( 25.0 / 360 ) * 3.14159 * 2;      /* use 25 degrees     */
    target_height = HEIGHT;

    error = FT_Init_FreeType( &library );              /* initialize library */
    if (error){
        fprintf(stdout, "error: FT_Init_FreeType {%x}\n", error);
    } 

    error = FT_New_Face( library, filename, 0, &face );/* create face object */
    if (error){
        fprintf(stdout, "error: FT_New_Face {%x}\n", error);
    } 
    // Ensure an unicode characater map is loaded
    error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    if (error){
        fprintf(stdout, "error: FT_Select_Charmap {%x}\n", error);
    } 
    error = FT_Set_Char_Size( face,  WIDTH * 64, HEIGHT * 64, 100, 0 );                /* set character size */
    if (error){
        fprintf(stdout, "error: FT_Set_Char_Size {%x}\n", error);
    } 

    slot = face->glyph;

    matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
    matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
    matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
    matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

    
    pen.x = 0;//WIDTH;
    pen.y = 0;//( target_height - HEIGHT ) * 64;
    FT_Set_Transform( face, &matrix, &pen );

    int img_width = 0;
    int img_height = 0;
    int control_cnt, max_charcode;
    int non_control_cnt = count_chars(face, &img_width, &img_height, &control_cnt, &max_charcode);
    fprintf(stdout, "non control glyphs: {%d}\n", non_control_cnt);
    fprintf(stdout, "control glyphs: {%d}\n", control_cnt);
    fprintf(stdout, "img width: {%d}\n", img_width);
    fprintf(stdout, "img height: {%d}\n", img_height);
    fprintf(stdout, "max charcode: {%d}\n", max_charcode);
    
    int to_alloc = non_control_cnt*img_width*img_height*sizeof(unsigned char);
    unsigned char* images = malloc(to_alloc);
    if (images == NULL){
        fprintf(stderr, "Malloc fault, to alloc {%d}", to_alloc);
    }
    OBS_char* characters = malloc(non_control_cnt*sizeof(OBS_char));
    OBS_char** charcode_to_character = malloc((max_charcode+1)*sizeof(OBS_char*));

    FT_UInt gid;
    FT_ULong charcode = FT_Get_First_Char(face, &gid);
    int cnt=0;
    unsigned char* img = images;
    OBS_char* ch = characters;
    while (gid != 0)
    {
        while (is_control(charcode)) {
            charcode = FT_Get_Next_Char(face, charcode, &gid);
            if (gid == 0) {
                break;
            }
        }
        if (gid == 0) {
            break;
        }
        ch->charcode = charcode;
        //fprintf(stdout, "%s:\n",to_unicode_string(charcode));
        error = FT_Load_Glyph(face, gid, FT_LOAD_RENDER);
        if (error){
            fprintf(stdout, "error: FT_Load_Glyph {%x}\n", error);
        } else {
            draw_bitmap(img, &face->glyph->bitmap, face->glyph->bitmap_left, 0, img_width, img_height );
            //show_image(img, img_width, img_height);
            ch->image = img;
            ch->match = 0;
            img += img_width*img_height;
        }
        charcode_to_character[charcode] = ch;
        ch++;
        charcode = FT_Get_Next_Char(face, charcode, &gid);
        cnt++;
    } 

    fprintf(stdout, text);
    char * tt = text;
    while (*tt){
        int len = len_next_unicode(tt);
        FT_ULong t = unicode_string_to_charcode(tt);
        tt += len;
        OBS_char* ch1 = charcode_to_character[t];
        if (ch1==NULL){
            fprintf(stdout, "Could not convert {%s:%x} to character\n", to_unicode_string(t), t );
        }
        if (ch1->match != 0) {
            continue;
        }
        int min_diff = 1000000;
        OBS_char* min_char = 0;
        OBS_char* ch2 = characters;
        while (ch2 < (characters+non_control_cnt)){
            if (ch1->charcode == ch2->charcode){
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
//        fprintf(stdout, "{%s, %x} <=> ",to_unicode_string(ch1->charcode),ch1->charcode);
//        fprintf(stdout, "{%s, %x}:       (%d)\n",to_unicode_string(min_char->charcode),min_char->charcode, min_diff);
        ch1->match = min_char;
        ch1++;
    }
    tt = text;
    while (*tt ){
        int len = len_next_unicode(tt);
        FT_ULong t = unicode_string_to_charcode(tt);
        tt += len;
        OBS_char* ch = charcode_to_character[t];
        //show_image(ch->image, img_width, img_height);
        //show_image(ch->match->image, img_width, img_height);
        fprintf(stdout, "diff : {%d} {%d}\n", ch->charcode, ch->match->charcode);
        print_overlap(ch->image,ch->match->image, img_width, img_height);
    }
    fprintf(stdout,"\n");
    tt = text;
    fprintf(stdout, "%s => ", text);
    while (*tt ){
        int len = len_next_unicode(tt);
        FT_ULong t = unicode_string_to_charcode(tt);
        tt += len;
        OBS_char* ch = charcode_to_character[t];
        fprintf(stdout,"%s", to_unicode_string(ch->match->charcode));
    }
    fprintf(stdout,"\n");

    FT_Done_Face    ( face );
    FT_Done_FreeType( library );

    return 0;
}

/* EOF */
