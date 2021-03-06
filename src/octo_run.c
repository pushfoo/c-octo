/**
*
*  Octo Run
*
*  a simple frontend for the c-octo emulator and compiler.
*  the default keyboard map is similar to web-octo,
*  with arrows and space aliased to ASWD and E.
*
*  if a gamepad is detected, it will be best-effort
*  mapped to ASWD (axes) and E/Q (buttons).
*
*   i - interrupt/resume
*   o - single step (while interrupted)
*   m - toggle monitor display
*  ^f - toggle fullscreen mode
*
*  esc or ` exits the program.
*
**/

#include "octo_emulator.h"
#include "octo_compiler.h"
#include "octo_cartridge.h"
#include <SDL.h>
#include "octo_util.h"

octo_program* prog=NULL;
octo_emulator emu;

int main(int argc, char* argv[]){
  if(argc<2){
    printf("octo-run v%s\n",VERSION);
    printf("usage: %s <source>\nwhere <source> is a .ch8 or .8o\n",argv[0]);
    return 0;
  }
  octo_load_program(&ui,&emu,&prog,argv[1]);

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
  SDL_Window  *win=SDL_CreateWindow("Octo-Run",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,ui.win_width*ui.win_scale,ui.win_height*ui.win_scale,SDL_WINDOW_SHOWN);
  SDL_Renderer*ren=SDL_CreateRenderer(win,-1, ui.software_render?SDL_RENDERER_SOFTWARE:SDL_RENDERER_ACCELERATED);
  SDL_Texture *screen=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,128,128); // oversized for rotation
  SDL_Texture *overlay=NULL;
  SDL_SetWindowFullscreen(win,ui.windowed?0:SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_AddTimer((1000/60),tick,NULL);
  SDL_JoystickEventState(SDL_ENABLE);
  SDL_Joystick*joy=NULL;
  audio_init(&emu);

  SDL_Event e;
  while(SDL_WaitEvent(&e)){
    if(e.type==SDL_QUIT)break;
    events_joystick(&emu,&joy,&e);
    if(e.type==SDL_KEYDOWN){
      int code=e.key.keysym.sym;
      for(int z=0;z<(int)(sizeof(keys)/sizeof(key_mapping));z++)if(keys[z].k==code)emu.keys[keys[z].v]=1;
    }
    if(e.type==SDL_KEYUP){
      int code=e.key.keysym.sym;
      for(int z=0;z<(int)(sizeof(keys)/sizeof(key_mapping));z++)if(keys[z].k==code){
        emu.keys[keys[z].v]=0;
        if(emu.wait){emu.v[(int)emu.wait_reg]=keys[z].v;emu.wait=0;}
      }
      if(code==SDLK_ESCAPE||code==SDLK_BACKQUOTE)break;
      if(code==SDLK_m)ui.show_monitors=!ui.show_monitors;
      if(emu.halt){
        if(code==SDLK_i)emu.halt=0;
        if(code==SDLK_o){emu.dt=emu.st=0;octo_emulator_instruction(&emu);snprintf(emu.halt_message,OCTO_HALT_MAX,"Single Stepping");}
      }
      else{
        if(code==SDLK_i){emu.halt=1;snprintf(emu.halt_message,OCTO_HALT_MAX,"User Interrupt");}
      }
      if(code==SDLK_f&&e.key.keysym.mod&(KMOD_LCTRL|KMOD_RCTRL|KMOD_LGUI|KMOD_RGUI)){
        ui.windowed=!ui.windowed;
        SDL_SetWindowFullscreen(win,ui.windowed?0:SDL_WINDOW_FULLSCREEN_DESKTOP);
      }
    }
    if(e.type==SDL_USEREVENT){
      emu_step(&emu,prog);
      SDL_FlushEvent(SDL_USEREVENT); // don't queue up repaints if we're running slow!

      int bgcolor=emu.options.colors[emu.st>0?OCTO_COLOR_SOUND: OCTO_COLOR_BACKGROUND];
      SDL_SetRenderDrawColor(ren,(bgcolor>>16)&0xFF,(bgcolor>>8)&0xFF,bgcolor&0xFF,0xFF);
      SDL_RenderClear(ren);

      int dw, dh, ow=0, oh=0;
      SDL_GetWindowSize(win,&dw,&dh);
      if(overlay!=NULL)SDL_QueryTexture(overlay,NULL,NULL,&ow,&oh);
      if(overlay==NULL||ow!=(dw/ui.win_scale)||oh!=(dh/ui.win_scale)){
        if(overlay!=NULL)SDL_DestroyTexture(overlay);
        overlay=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,(dw/ui.win_scale),(dh/ui.win_scale));
      }
      octo_ui_run(&emu,prog,&ui,win,ren,screen,overlay);
    }
  }
  SDL_Quit();
  return 0;
}
