#include "achievements.h"
#include "achievement_overlay.h"
#include "achievement_sound.h"
#include "common/launcher_gl.h"
#include "snes_overlay_draw.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"
static unsigned char *card,*badge;
static int cw,ch,bw,bh,last=-1;
static uint32_t panel[540*180];
static unsigned char *font_data;
static stbtt_fontinfo font;
static int font_attempted, font_ready;
static void load_font(void) {
    if(font_attempted) return;
    font_attempted=1;
    FILE *f=fopen("assets/fonts/LatoLatin-Bold.ttf","rb");
    if(!f) return;
    if(fseek(f,0,SEEK_END)!=0) { fclose(f); return; }
    long size=ftell(f); rewind(f);
    if(size<=0 || size>4*1024*1024) { fclose(f); return; }
    font_data=malloc((size_t)size);
    if(font_data && fread(font_data,1,(size_t)size,f)==(size_t)size)
        font_ready=stbtt_InitFont(&font,font_data,stbtt_GetFontOffsetForIndex(font_data,0));
    fclose(f);
    if(!font_ready) { free(font_data); font_data=NULL; }
}
static float label_width(const char *s,float scale) {
    float width=0;
    while(*s) {
        int advance,bearing;
        stbtt_GetCodepointHMetrics(&font,(unsigned char)*s,&advance,&bearing);
        width+=advance*scale;
        if(s[1]) width+=stbtt_GetCodepointKernAdvance(&font,(unsigned char)*s,(unsigned char)s[1])*scale;
        s++;
    }
    return width;
}
/* Proportional, antialiased labels stay legible when the panel is displayed
 * at half size; long names no longer fall back to an 8-pixel bitmap font. */
static void label(int x,int y,int width,const char *s,float height,uint32_t color) {
    load_font();
    if(!font_ready) {
        int scale=height>=28 ? 2 : 1;
        snes_ovl_draw_text(panel,540,180,x+(width-(int)strlen(s)*8*scale)/2,y,s,color,scale);
        return;
    }
    float scale=stbtt_ScaleForPixelHeight(&font,height);
    float text_width=label_width(s,scale);
    if(text_width>width) scale*=width/text_width;
    float pen=x+(width-label_width(s,scale))/2;
    int ascent,descent,gap;
    stbtt_GetFontVMetrics(&font,&ascent,&descent,&gap);
    int baseline=y+(int)(ascent*scale);
    while(*s) {
        int w,h,ox,oy,advance,bearing;
        unsigned char *bitmap=stbtt_GetCodepointBitmap(&font,0,scale,(unsigned char)*s,&w,&h,&ox,&oy);
        for(int row=0;bitmap && row<h;row++) for(int col=0;col<w;col++) {
            int dx=(int)pen+ox+col,dy=baseline+oy+row;
            if(dx<0 || dx>=540 || dy<0 || dy>=180) continue;
            unsigned alpha=bitmap[row*w+col];
            uint32_t old=panel[dy*540+dx];
            unsigned r=(((color>>16)&255)*alpha+((old>>16)&255)*(255-alpha))/255;
            unsigned g=(((color>>8)&255)*alpha+((old>>8)&255)*(255-alpha))/255;
            unsigned b=((color&255)*alpha+(old&255)*(255-alpha))/255;
            panel[dy*540+dx]=0xff000000u|(r<<16)|(g<<8)|b;
        }
        stbtt_FreeBitmap(bitmap,NULL);
        stbtt_GetCodepointHMetrics(&font,(unsigned char)*s,&advance,&bearing);
        pen+=advance*scale;
        if(s[1]) pen+=stbtt_GetCodepointKernAdvance(&font,(unsigned char)*s,(unsigned char)s[1])*scale;
        s++;
    }
}
static void image_rect(unsigned char *image,int iw,int ih,int x,int y,int w,int h) {
    if(!image || iw<=0 || ih<=0) return;
    for(int dy=0;dy<h;dy++) for(int dx=0;dx<w;dx++) {
        const unsigned char *p=image+((dy*ih/h)*iw+dx*iw/w)*4;
        if(x+dx<0 || x+dx>=540 || y+dy<0 || y+dy>=180) continue;
        panel[(y+dy)*540+x+dx]=((uint32_t)((image==card && p[0]<8 && p[1]<8 && p[2]<8) ? 0 : p[3])<<24)|(p[0]<<16)|(p[1]<<8)|p[2];
    }
}
static void text_center(int x,int y,int width,const char *s,int scale) {
    int w=(int)strlen(s)*8*scale;
    snes_ovl_draw_text(panel,540,180,x+(width-w)/2,y,s,0xffeee6cf,scale);
}
static void rebuild(int index) {
    const ScivAchievement *a=sciv_achievement(index);
    if(!card) card=launcher_image_load_rgba("assets/castlevania/achievements/card.png",&cw,&ch);
    if(badge) launcher_image_free(badge);
    char path[160]; snprintf(path,sizeof(path),"assets/castlevania/achievements/%s.png",a->badge);
    badge=a->id ? launcher_image_load_rgba(path,&bw,&bh) : NULL;
    memset(panel,0,sizeof(panel));
    image_rect(card,cw,ch,0,0,540,180);
    /* Keep the supplied ornament and G medallion, but give the achievement
     * name priority over the generic unlock heading. */
    snes_ovl_fill_rect(panel,540,180,138,44,361,66,0xff041431);
    snes_ovl_fill_rect(panel,540,180,152,116,282,25,0xff041431);
    snes_ovl_fill_rect(panel,540,180,470,111,25,24,0xff041431);
    image_rect(badge,bw,bh,45,63,70,65);
    label(140,49,357,"Achievement Unlocked!",18,0xffa9c8e8);
    label(140,74,357,a->name,36,0xffeee6cf);
    /* Three short description lines, never overflow into the score. */
    const char *p=a->description;
    for(int line=0;line<3 && *p;line++) {
        char buf[35]; size_t n=strlen(p); if(n>34) { n=34; while(n && p[n]!=' ') n--; if(!n) n=34; }
        memcpy(buf,p,n); buf[n]=0;
        text_center(152,116+line*8,282,buf,1);
        p+=n; while(*p==' ') p++;
    }
    char score[8]; snprintf(score,sizeof(score),"%u",a->points);
    text_center(470,118,26,score,a->points<100 ? 2 : 1);
}
int sciv_achievement_overlay_scale(void) { return sciv_achievements_toast()>=0 ? 3 : 1; }
void sciv_achievement_overlay(uint8_t *dst,int pitch,int w,int h) {
    int index=sciv_achievements_toast(); if(index<0) { last=-1; return; }
    if(index!=last) { rebuild(index); last=index; sciv_achievement_sound_play(); }
    int width=w*3/4; if(width>540) width=540;
    width/=2; /* Half the previous card width and height. */
    int height=width/3,margin=w/80;
    snes_ovl_blit_panel_rect(dst,pitch,w,h,panel,540,180,w-width-margin,h-height-margin,width,height);
}

/* Title-screen browser, matching Kickle's six-row list and selected-detail
 * layout. Reuse the bundled font and badges; no launcher window is opened. */
enum { BROWSER_W=768,BROWSER_H=600 };
static uint32_t browser[BROWSER_W*BROWSER_H];
static unsigned char *browser_badges[SCIV_ACHIEVEMENT_COUNT];
static int browser_bw[SCIV_ACHIEVEMENT_COUNT],browser_bh[SCIV_ACHIEVEMENT_COUNT];
static int browser_loaded[SCIV_ACHIEVEMENT_COUNT];
static void browser_text(int x,int y,const char *s,float height,int max_width,uint32_t color) {
    load_font();
    if(!font_ready) {snes_ovl_draw_text(browser,BROWSER_W,BROWSER_H,x,y,s,color,height>=24 ? 2 : 1);return;}
    float scale=stbtt_ScaleForPixelHeight(&font,height);
    float width=label_width(s,scale);if(width>max_width)scale*=max_width/width;
    float pen=x;int ascent,descent,gap;stbtt_GetFontVMetrics(&font,&ascent,&descent,&gap);
    int baseline=y+(int)(ascent*scale);
    while(*s) {
        int w,h,ox,oy,advance,bearing;
        unsigned char *bitmap=stbtt_GetCodepointBitmap(&font,0,scale,(unsigned char)*s,&w,&h,&ox,&oy);
        for(int row=0;bitmap && row<h;row++)for(int col=0;col<w;col++) {
            int dx=(int)pen+ox+col,dy=baseline+oy+row;
            if(dx<0 || dx>=BROWSER_W || dy<0 || dy>=BROWSER_H)continue;
            unsigned a=bitmap[row*w+col];uint32_t old=browser[dy*BROWSER_W+dx];
            unsigned r=(((color>>16)&255)*a+((old>>16)&255)*(255-a))/255;
            unsigned g=(((color>>8)&255)*a+((old>>8)&255)*(255-a))/255;
            unsigned b=((color&255)*a+(old&255)*(255-a))/255;
            browser[dy*BROWSER_W+dx]=0xff000000u|(r<<16)|(g<<8)|b;
        }
        stbtt_FreeBitmap(bitmap,NULL);stbtt_GetCodepointHMetrics(&font,(unsigned char)*s,&advance,&bearing);
        pen+=advance*scale;if(s[1])pen+=stbtt_GetCodepointKernAdvance(&font,(unsigned char)*s,(unsigned char)s[1])*scale;s++;
    }
}
static void browser_wrap(int x,int y,const char *s,int width,float height,int lines,uint32_t color) {
    load_font();float scale=font_ready ? stbtt_ScaleForPixelHeight(&font,height) : 1;
    for(int row=0;row<lines && *s;row++) {
        char line[128];size_t n=0,last_space=0;
        while(s[n] && n<sizeof(line)-1) {
            line[n]=s[n];line[n+1]=0;
            float size=font_ready ? label_width(line,scale) : (float)(n+1)*8;
            if(size>width && n)break;
            if(s[n]==' ')last_space=n;n++;
        }
        if(s[n] && last_space)n=last_space;
        if(!n)n=1;memcpy(line,s,n);line[n]=0;
        browser_text(x,y+row*(int)(height+7),line,height,width,color);
        s+=n;while(*s==' ')s++;
    }
}
static void browser_icon(int index,int x,int y,int size) {
    if(!browser_loaded[index]) {
        browser_loaded[index]=1;
        const ScivAchievement *a=sciv_achievement(index);
        char path[160];snprintf(path,sizeof(path),"assets/castlevania/achievements/%s.png",a->badge);
        if(a->id)browser_badges[index]=launcher_image_load_rgba(path,&browser_bw[index],&browser_bh[index]);
        else {
            if(!card)card=launcher_image_load_rgba("assets/castlevania/achievements/card.png",&cw,&ch);
        }
    }
    const unsigned char *pixels=browser_badges[index];int iw=browser_bw[index],ih=browser_bh[index];
    int earned=sciv_achievement_unlocked(index);
    for(int dy=0;dy<size;dy++)for(int dx=0;dx<size;dx++) {
        const unsigned char *p=NULL;
        if(pixels && iw>0 && ih>0)p=pixels+((dy*ih/size)*iw+dx*iw/size)*4;
        else if(!sciv_achievement(index)->id && card && cw>0 && ch>0) {
            int sx=cw*99/1000+dx*(cw*95/1000)/size,sy=ch*359/1000+dy*(ch*315/1000)/size;
            p=card+(sy*cw+sx)*4;
        }
        if(!p)continue;
        unsigned r=p[0],g=p[1],b=p[2];
        if(!earned) {unsigned gray=(r*3+g*6+b)/10;r=gray*45/100;g=gray*45/100;b=gray*50/100;}
        uint32_t old=browser[(y+dy)*BROWSER_W+x+dx];unsigned a=p[3];
        r=(r*a+((old>>16)&255)*(255-a))/255;g=(g*a+((old>>8)&255)*(255-a))/255;b=(b*a+(old&255)*(255-a))/255;
        browser[(y+dy)*BROWSER_W+x+dx]=0xff000000u|(r<<16)|(g<<8)|b;
    }
    if(!pixels && (sciv_achievement(index)->id || !card)) {
        snes_ovl_fill_rect(browser,BROWSER_W,BROWSER_H,x,y,size,size,0xff3a3540);
        browser_text(x+size/4,y+size/4,"G",size/2,size/2,0xffe1bd7b);
    }
}
void sciv_achievement_browser(uint8_t *dst,int pitch,int w,int h,int selected) {
    if(selected<0 || selected>=SCIV_ACHIEVEMENT_COUNT)selected=0;
    int earned=0;unsigned total=0;
    for(int i=0;i<SCIV_ACHIEVEMENT_COUNT;i++) {earned+=sciv_achievement_unlocked(i);total+=sciv_achievement(i)->points;}
    for(int i=0;i<BROWSER_W*BROWSER_H;i++)browser[i]=0xff101018;
    snes_ovl_fill_rect(browser,BROWSER_W,BROWSER_H,0,0,BROWSER_W,3,0xffa78354);
    browser_text(24,18,"Achievements",36,360,0xffffdf9b);
    char progress[80];snprintf(progress,sizeof(progress),"%d / %d unlocked    %u / %u G",earned,SCIV_ACHIEVEMENT_COUNT,sciv_achievements_score(),total);
    browser_text(24,63,progress,23,720,0xffc8c8bc);
    snes_ovl_fill_rect(browser,BROWSER_W,BROWSER_H,24,100,352,418,0xff181820);
    snes_ovl_fill_rect(browser,BROWSER_W,BROWSER_H,390,100,354,418,0xff181820);
    int first=selected/6*6;
    for(int row=0;row<6 && first+row<SCIV_ACHIEVEMENT_COUNT;row++) {
        int i=first+row,y=108+row*67;
        const ScivAchievement *a=sciv_achievement(i);
        if(i==selected)snes_ovl_fill_rect(browser,BROWSER_W,BROWSER_H,28,y,344,63,0xff413323);
        browser_icon(i,36,y+7,48);
        browser_text(96,y+7,a->name,24,266,i==selected ? 0xffffdf9b : 0xffd4d0c8);
        snprintf(progress,sizeof(progress),"%s   %u G",sciv_achievement_unlocked(i) ? "Unlocked" : "Locked",a->points);
        browser_text(96,y+36,progress,18,266,0xffa9a5a0);
    }
    const ScivAchievement *a=sciv_achievement(selected);
    browser_icon(selected,414,120,112);
    browser_wrap(414,251,a->name,304,32,2,0xffffdf9b);
    snprintf(progress,sizeof(progress),"%s    %u G",sciv_achievement_unlocked(selected) ? "UNLOCKED" : "LOCKED",a->points);
    browser_text(414,333,progress,23,306,sciv_achievement_unlocked(selected) ? 0xff99dca0 : 0xffa9a5a0);
    browser_wrap(414,378,a->description,302,25,4,0xffd4d0c8);
    snprintf(progress,sizeof(progress),"Page %d / %d",selected/6+1,(SCIV_ACHIEVEMENT_COUNT+5)/6);
    browser_text(24,533,progress,22,250,0xffe1bd7b);
    browser_text(24,565,"Up/Down: Select    Left/Right: Page    B: Back",22,720,0xffb4b0a8);
    /* Cover the original Play Select text around the browser as well. */
    for(int y=0;y<h;y++) {
        uint32_t *row=(uint32_t *)(dst+y*pitch);
        for(int x=0;x<w;x++)row[x]=0xff090910;
    }
    int rw=w*94/100,rh=h*90/100;
    snes_ovl_blit_panel_rect(dst,pitch,w,h,browser,BROWSER_W,BROWSER_H,(w-rw)/2,(h-rh)/2,rw,rh);
}
