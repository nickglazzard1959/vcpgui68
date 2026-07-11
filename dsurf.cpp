// dsurf.cpp - Display Surface component for vcpgui.
// =================================================
//
// This a an SDL 2 based program that opens a window on which vcpgui
// can draw UI components and from which it can receive user interaction 
// event notifications.
//
// Pre-requisites:
//   sudo apt install libsdl2-dev
//   sudo apt install libsdl2-gfx-dev
//   sudo apt install libsdl2-ttf-dev
//   sudo apt install libsdl2-image-dev
//
// Build:
//   g++ -O0 -g dsurf.cpp -o dsurf -lSDL2 -lSDL2_gfx -lSDL2_ttf -lSDL2_image
//
// Nick Glazzard 2026.
// -------------------

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <cctype>
#include <fstream>
#include <cmath>

// N.B. You have to get this from the "releases" page of the CLI11 project
// on Github. DO NOT waste your time cloning the thing. This is all you need.
#include "CLI11.hpp"

// List of things to be drawn.
typedef struct
{
  SDL_mutex* mutex;
  std::vector<std::string> prims;
} DISPLAY_LIST;

// Command codes for each command string.
typedef enum {
  GRF_clear,                        // r, g, b, a
  GRF_rectangle,                    // x1, y1, x2, y2, r, g, b, a
  GRF_rounded_rectangle,            // x1, y1, x2, y2, rad, r, g, b, a
  GRF_rectangle_filled,             // x1, y1, x2, y2, r, g, b, a
  GRF_rounded_rectangle_filled,     // x1, y1, x2, y2, rad, r, g, b, a
  GRF_line,                         // x1, y1, x2, y2, r, g, b, a
  GRF_thick_line,                   // x1, y1, x2, y2, w, r, g, b, a
  GRF_circle,                       // xc, yc, rad, r, g, b, a
  GRF_circle_filled,                // xc, yc, rad, r, g, b, a
  GRF_arc,                          // xc, yc, r, sa, ea, r, g, b, a
  GRF_ellipse,                      // xc, yc, rx, ry, r, g, b, a
  GRF_ellipse_filled,               // xc, yc, rx, ry, r, g, b, a
  GRF_text,                         // x, y, string, centered, dy, r, g, b, a
  GRF_text_in_font,                 // x, y, string, centered, dy, r, g, b, ifont
  GRF_sub_texture,                  // name, x, y, downscale, centered, rotang, rotx, roty

  GRF_show_display,                 // <none>
  GRF_show_display_sync,            // <none>

  CFG_reset_display,                // <none>
  CFG_use_main,                     // <none>
  CFG_use_overlay,                  // <none>
  CFG_load_font,                    // filename, size, font number 0:MXF-1
  CFG_load_texture,                 // filename
  CFG_sense_rect,                   // tag, x, y, sx, sy, events
  CFG_debug,                        // <none>
  CFG_remove,                       // x, y
  CFG_NOP                           // Do nothing. Used to nullify CFG commands once processed.
} CMD_CODE;

typedef struct {
  CMD_CODE code;
  int n_ints;
  int n_strings;
} CMD_INFO;

typedef std::map<std::string, CMD_INFO> CMD_MAP;

typedef struct
{
  std::string tex_name; // Name to refer to the sub-texture.
  int x, y;             // Top left pixel,line of sub-texture in texture image.
  int sx, sy;           // Size in x and y of the sub-texture.
} SUB_TEXTURE_DATA;

typedef std::map<std::string, SUB_TEXTURE_DATA> ATLAS_MAP;

typedef struct
{
  SDL_Texture* texture; // Full texture image,
  ATLAS_MAP atlas;      // Sub-texture atlas.
  bool valid;           // Loaded successfully.
} TEXTURE_ATLAS;

const int SR_DOWN_EVENT = 1;
const int SR_UP_EVENT = 2;
const int SR_MOTION_EVENT = 4;
const int SR_ANGLE_EVENT = 8;
const int SR_WHEEL_EVENT = 16;

typedef struct
{
  int x, y;                // Top left of sense rectangle.
  int sx, sy;              // Width and height of sense rectangle.
  std::string tag;         // Tag to send on selection.
  int event_bits;          // Events to respond to.
  int x_down, y_down;      // Position of last down event.
} SENSE_RECT;

const int MXF = 5;         // Maximum number of loaded fonts.

typedef struct
{
  bool debug_mode;         // Debug mode.
  bool keep_on_eof;        // Do not close window on EOF on stdin.
  bool fast_update;        // Swap buffers after updates, do not wait for show_display.
  TTF_Font* fonts[MXF];    // Loaded TrueType fonts.
  TTF_Font* font;          // Current font as pointer.
  int cur_font;            // Current font as index in to fonts.
  TEXTURE_ATLAS tex_atlas; // Texture atlas.
  std::vector<SENSE_RECT> sense_rects; // Areas in which we want to know about events.
  CMD_MAP cmdmap;          // Command name to code map.
  std::string fontdir;     // Where to look for TrueType fonts;
} DRAW_STATE;

typedef struct
{
  DRAW_STATE* dstate;  // Draw state, used for debug flag.
  DISPLAY_LIST* dlist; // Display list.
  DISPLAY_LIST* olist; // Overlay display list.
} THREAD_DATA;

// Forward declarations.
void remove_from_display_list( DISPLAY_LIST* dlist,
                               DRAW_STATE* dstate,
                               CMD_MAP& cmdmap,
                               int x, int y );
std::vector<int> extract_int_args( std::string primdesc );

static inline Uint8 clamp8( int v )
//---------------------------------
// Convert v to valid unsigned 8 bit.
// Don't use std::clamp as that needs C++17 and still needs a cast.
{
  v = (v > 255) ? 255 : (v < 0) ? 0 : v ;
  return (Uint8)v;
}

int read_thread( void* data )
//---------------------------
// Read commands from stdin on this thread.
// Put the command strings on to the display list "as is".
// Any errors here will simply exit the program.
{
  const int MAXLINE = 128;
  char input_line[MAXLINE];
  THREAD_DATA* td_p = (THREAD_DATA*) data;
  DISPLAY_LIST* dlist_p = td_p->dlist;
  DISPLAY_LIST* olist_p = td_p->olist;
  DRAW_STATE* dstate_p = td_p->dstate;
  DISPLAY_LIST* clist_p = dlist_p;
  
  SDL_Log("Read thread started\n");
  //fprintf(stderr, "read_thread(), dlist_p=%p, olist_p=%p\n",dlist_p,olist_p);
  while( true ){

    // Block here until a line is sent.
    char* pinchars = fgets(input_line, MAXLINE, stdin);

    // Add a string containing the line to the display list primitives.
    if( NULL != pinchars ){
      if( SDL_LockMutex(dlist_p->mutex) < 0 ){
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to lock mutex in read_thread(), error: %s.\n", SDL_GetError());
        exit(1);
      }
      
      else{
        std::string string_input_line = input_line;
        if( dstate_p->debug_mode )
          fprintf(stderr, "read_thread() input line = [%s]\n", input_line);
        if( string_input_line == "$*use_overlay$\n" )  // Select the overlay list.
          clist_p = olist_p;
        else if( string_input_line == "$*use_main$\n" ) // Select the main list.
          clist_p = dlist_p;
        else if( string_input_line == "$*reset_display$\n" )  // Empty the display list.
          clist_p->prims.clear();
        else if( string_input_line.rfind("$*remove$",0) != std::string::npos ){ // Remove. Complicated.
          std::vector<int> args = extract_int_args(string_input_line);
          // Remove from the overlay list.
          remove_from_display_list( olist_p,
                                    dstate_p,
                                    dstate_p->cmdmap,
                                    args[0], args[1] );
          // And remove from the main list. This prevents "ghosts" for certain shapes.
          remove_from_display_list( dlist_p,
                                    dstate_p,
                                    dstate_p->cmdmap,
                                    args[0], args[1] );
        }
        else
          clist_p->prims.push_back(string_input_line);   // Add the primitive to the display list.
        
        if( SDL_UnlockMutex(dlist_p->mutex) < 0 ){
          SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                          "Failed to unlock mutex in read_thread(), error: %s.\n", SDL_GetError());
          exit(1);
        }
      }
    }

    // EOF or something went wrong.
    else{
      if( dstate_p->keep_on_eof ){
        SDL_Log("EOF on stdin, but in no exit mode. Not closing window.\n");
        return 0;
      }
      else{
        if( dstate_p->debug_mode && feof(stdin) ){
          SDL_Log("EOF on stdin in debug mode. Waiting 200 seconds before exit.\n");
          sleep(200);
        }
        else
          SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                          "Failed to fgets() from stdin. Giving up.\n");
        exit(1);
      };
    }

    // Push a user event on to the event queue so the updated
    // display list gets drawn.
    SDL_Event user_event;
    SDL_zero(user_event);
    user_event.type = SDL_USEREVENT;
    user_event.user.code = 1;
    user_event.user.data1 = NULL;
    user_event.user.data2 = NULL;
    SDL_PushEvent(&user_event);
    
  } // eternal loop.
  return 0;
}

std::vector<int> extract_int_args( std::string primdesc )
//-------------------------------------------------------
// Return a vector of integer arguments found in command
// string primdesc. Remember integers can be negative!
{
  std::vector<int> arguments;
  int value = 0;
  int sign = 1;
  bool in_digits = false;
  bool in_string = false;

  // Examine string chars for decimal digits. Assemble integers.
  for( char c : primdesc ){
    if( c == '$' ){
      in_string = ! in_string;
    }
    else{
      if( ! in_string ){
        if( isdigit(c) || (c == '-') ){
          if( c == '-' )
            sign = -1;
          else
            value = 10 * value + (c - '0');
          in_digits = true;
        }
        else{
          if( in_digits ){
            arguments.push_back(sign * value);
            value = 0;
            sign = 1;
            in_digits = false;
          }
        }
      } // not in a string argument.
    } // char is not $.
  } // over chars of primdesc.

  // Trailing number case.
  if( in_digits ){
    arguments.push_back(sign * value);
  }

  return arguments;
}

std::vector<std::string> extract_string_args( std::string primdesc )
//------------------------------------------------------------------
// Return a vector of atring arguments found in command string
// primdesc.
{
  std::string new_string;
  std::vector<std::string> arguments;
  bool in_string = false;

  // String arguments begin and end with $.
  for( char c : primdesc ){
    if( c == '$' ){
      if( in_string ){
        arguments.push_back(new_string);
        new_string = "";
        in_string = false;
      }
      else{
        in_string = true;
      }
    }

    else{
      if( in_string ){
        new_string += c;
      }
    }
  } // over chars of primdesc.

  return arguments;
}

void send_sync()
//--------------
// Send a synchronisation message to the client.
{
  fputs("SYNC\n", stdout);
  fflush(stdout);
}

int draw_text_ttf( SDL_Renderer* renderer,
                   TTF_Font* font,
                   int r, int g, int b,
                   std::string chars,
                   int x, int y,
                   bool center,
                   int dy )
//-----------------------------------------------
// Draw a text string using a pre-loaded TrueType font.
{
  SDL_Color color = {clamp8(r), clamp8(g), clamp8(b)};
  SDL_Surface* surface = TTF_RenderText_Blended(font, chars.c_str(), color);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

  int tex_w = 0;
  int tex_h = 0;
  SDL_QueryTexture(texture, NULL, NULL, &tex_w, &tex_h);
  int x_off = ( center ) ? tex_w / 2 : 0 ;
  int y_off = ( center ) ? tex_h / 2 : 0 ;
  SDL_Rect dstrect = {x-x_off, y-y_off-dy, tex_w, tex_h};
  SDL_RenderCopy(renderer, texture, NULL, &dstrect);

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);

  return 0;
}

int draw_text_8x8( SDL_Renderer* renderer,
                   int r, int g, int b, int a,
                   std::string chars,
                   int x, int y,
                   bool center,
                   int dy )
//-----------------------------------------------
// Draw a text string using an 8x8 dot font built into
// the SDL_gfx library.
{
  int x_off = 8 * ( ( center ) ? (int)chars.size() / 2 : 0 );
  int y_off = ( center ) ? 8 / 2 : 0 ;
  r = clamp8(r);
  g = clamp8(g);
  b = clamp8(b);
  a = clamp8(a);
  stringRGBA(renderer, x-x_off, y-y_off-dy, chars.c_str(), r, g, b, a);

  return 0;
}

SDL_Texture* get_image_as_texture( SDL_Renderer* renderer,
                                   const char* filename )
//---------------------------------------------------------
// Read an image file and make a texture from its contents.
{
  SDL_Texture* texture = IMG_LoadTexture(renderer, filename);
  if( NULL == texture ){
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Couldn't load image from: %s, error: %s\n", filename, SDL_GetError());
    return NULL;
  }
  
  return texture;
}

int draw_texture_image( SDL_Renderer* renderer,
                        SDL_Texture* image,
                        int x, int y,
                        bool center )
//--------------------------------------------------
// Draw a texture mapped rectangle to show an entire image.
{
  if( NULL == image )
    return 1;

  int tex_w = 0;
  int tex_h = 0;
  SDL_QueryTexture(image, NULL, NULL, &tex_w, &tex_h);

  int x_off = ( center ) ? tex_w / 2 : 0 ;
  int y_off = ( center ) ? tex_h / 2 : 0 ;
  SDL_Rect dstrect = {x-x_off, y-y_off, tex_w, tex_h};
  SDL_RenderCopy(renderer, image, NULL, &dstrect);  
  
  return 0;
}

int draw_texture_sub_image( SDL_Renderer* renderer,
                            SDL_Texture* image,
                            int src_x, int src_y,
                            int src_w, int src_h,
                            int dst_x, int dst_y,
                            int down_scale,
                            bool center,
                            int rotang, int rx, int ry )
//---------------------------------------------------
// Draw a sub-rectangle of a texture image. The source rectangle
// has its top left at src_x, src_y, size src_w, src_h. This is
// drawn at dst_x, dst_y (either top left, or center), downsampled
// by down_scale.
{
  if( NULL == image )
    return 1;

  SDL_Rect srcrect = {src_x, src_y, src_w, src_h};
  int dst_w = src_w / down_scale;
  int dst_h = src_h / down_scale;

  int x_off = ( center ) ? dst_w / 2 : 0 ;
  int y_off = ( center ) ? dst_h / 2 : 0 ;
  SDL_Rect dstrect = {dst_x-x_off, dst_y-y_off, dst_w, dst_h};

  if( 0 == rotang )
    SDL_RenderCopy(renderer, image, &srcrect, &dstrect);
  else{
    double angle = rotang;
    SDL_Point center = {rx, ry};
    SDL_RenderCopyEx(renderer, image, &srcrect, &dstrect, -angle, &center, SDL_FLIP_NONE);
  }
  
  return 0;
}

void ltrim(std::string& s)
//------------------------
// Remove leading white space from a string, in place.
// This doesn't seem to be in C++11 ... maybe one day ...
{
  s.erase(
          s.begin(),
          std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          })
          );
}

void rtrim(std::string& s)
//------------------------
// Remove trailing white space ...
{
  s.erase(
          std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
          }).base(),
          s.end()
          );
}

void trim(std::string& s)
//-----------------------
// Remove leading and trailing white space ...
{
  ltrim(s);
  rtrim(s);
}

void clean_up_texture_atlas( TEXTURE_ATLAS& tex_atlas )
//-----------------------------------------------------
// Try to free some heap associated with a texture atlas.
{
  if( tex_atlas.valid ){
    if( NULL != tex_atlas.texture )
      SDL_DestroyTexture(tex_atlas.texture);
    tex_atlas.atlas.clear();
    tex_atlas.valid = false;
  }
}

TEXTURE_ATLAS load_texture_atlas( SDL_Renderer* renderer, std::string filename )
//------------------------------------------------------------------------------
// Load a description of a texture image identifying named
// sub-textures within the image.
{
  TEXTURE_ATLAS tex_atlas;

  tex_atlas.valid = false;
  
  std::ifstream stin(filename);
  if( ! stin.is_open() )
    return tex_atlas;

  std::string line;
  int line_no = 0;
  while( std::getline(stin, line) ){
    trim(line);
    if( line[0] == '#' )
      continue;
    
    if( line_no == 0 ){
      std::string imagefilename = line;
      tex_atlas.texture = get_image_as_texture(renderer, imagefilename.c_str());
      if( NULL == tex_atlas.texture )
        return tex_atlas;
    }
    else{
      SUB_TEXTURE_DATA stdata;
      char c_tex_name[128];
      if( sscanf(line.c_str(), "%s %d %d %d %d", c_tex_name,
                 &stdata.x, &stdata.y, &stdata.sx, &stdata.sy) != 5 )
        return tex_atlas;
      stdata.tex_name = std::string(c_tex_name);

      tex_atlas.atlas[stdata.tex_name] = stdata;
      // fprintf(stderr, "%s,%d,%d,%d,%d\n",
      // stdata.tex_name.c_str(), stdata.x, stdata.y, stdata.sx, stdata.sy);
    }
        
    line_no += 1;
  }

  tex_atlas.valid = true;
  stin.close();
  return tex_atlas;
}

int draw_sub_texture_from_atlas( SDL_Renderer* renderer,
                                 TEXTURE_ATLAS tex_atlas,
                                 std::string tex_name,
                                 int dst_x, int dst_y,
                                 int down_scale, bool center,
                                 int rotang, int rx, int ry )
//------------------------------------------------------------
// Draw a named sub-texture from a texture atlas.
{
  if( ! tex_atlas.valid ){
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Texture atlas is invalid.\n");
    return 1;
  }

  if( tex_atlas.atlas.find(tex_name) == tex_atlas.atlas.end() ){
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Requested texture is not in the atlas: %s\n", tex_name.c_str());
    return 2;
  }

  SUB_TEXTURE_DATA stdata = tex_atlas.atlas[tex_name];
  
  return draw_texture_sub_image(renderer,
                                tex_atlas.texture,
                                stdata.x, stdata.y,
                                stdata.sx, stdata.sy,
                                dst_x, dst_y,
                                down_scale,
                                center,
                                rotang, rx, ry);
}

int load_TTF_font( std::string filename, int size, int ifont, DRAW_STATE& dstate )
//--------------------------------------------------------------------------------
// Load a TTF font at size in to font ifont.
{
  std::string fontname = dstate.fontdir + "/" + filename;

  // Ensure ifont is valid.
  if( (ifont < 0) || (ifont >= MXF) ){
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Invalid font number: %d", ifont);
    return 1;
  }

  // Try to load the new font.
  TTF_Font *new_font = TTF_OpenFont(fontname.c_str(), size);
  if( NULL == new_font ){
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Couldn't load font: %s", SDL_GetError());
    return 1;
  }

  // If that worked, free the old font, set the new font.
  if( NULL != dstate.fonts[ifont] )
    TTF_CloseFont(dstate.fonts[ifont]);
  dstate.fonts[ifont] = new_font;

  return 0;
}

int set_font( DRAW_STATE& dstate, int ifont )
//-------------------------------------------
// Select font by number. That font slot must have been loaded before.
{
  // Ensure ifont is valid.
  if( (ifont < 0) || (ifont >= MXF) ){
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Invalid font number: %d", ifont);
    return 1;
  }

  if( NULL == dstate.fonts[ifont] ){
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Font number: %d has not been loaded with a font", ifont);
    return 1;
  }

  dstate.cur_font = ifont;
  dstate.font = dstate.fonts[ifont];
  // fprintf(stderr, "dstate.cur_font = %d, dstate.font = %p\n",dstate.cur_font,dstate.font);
  return 0;
}

int in_sense_rect( int x, int y, std::vector<SENSE_RECT>& sense_rects,
                    int event_type,
                    std::string& tag )
//---------------------------------------------------------------------
// See if (x,y) lies inside a defined sense_rect. If so, return sense_rect index
// and the tag string for that rect. If not, return false. Note: event_type is
// the event that has occurred, translated into our SR_* numbers which are bit masks.
// We only want rects that have declared themselves as interested in the event that
// has occurred.
{
  // Linear search over sense_rects.
  for( size_t i=0; i<sense_rects.size(); i++ ){

    // See if the rect is interested in the event which has occurred.
    if( (event_type & sense_rects[i].event_bits) != 0 ){
      int dx = x - sense_rects[i].x;
      int dy = y - sense_rects[i].y;

      // See if the event has occurred inside this sense_rect.
      if( dx >= 0 && dx < sense_rects[i].sx && dy >= 0 && dy < sense_rects[i].sy ){
        
        // In the sense rectangle.
        // Track the coordinate at which the mouse button went down.
        if( event_type == SR_DOWN_EVENT ){
          sense_rects[i].x_down = x;
          sense_rects[i].y_down = y;
        }

        // Clear the last down coordinate on mouse button up.
        else if( event_type == SR_UP_EVENT ){
          sense_rects[i].x_down = -1;
          sense_rects[i].y_down = -1;
        }

        // For motion and angle events, only select rect if mouse went down in it.
        else if( (event_type & (SR_MOTION_EVENT | SR_ANGLE_EVENT)) != 0 ){
          if( sense_rects[i].x_down < 0 )
            continue;
        }

        // Return the tag identifying the sense_rect.
        tag = sense_rects[i].tag;
        return i;
      } // Inside rect.

      // Clear last down coordinate of all rects we are not in.
      else{
        sense_rects[i].x_down = -1;
        sense_rects[i].y_down = -1;
      } // Outside rect.
    } // Rect interested in event which has occurred.
  } // Over rects.

  // Not in any rect.
  return -1;
}

int sense_rect_angle( SENSE_RECT& sense_rect, int x, int y )
//----------------------------------------------------------
// Calculate the angle of (x,y) measured from the center of a sense rect.
// Correct the angle so it is meaasured wrt +y axis. Switch sign so +ve
// is clockwise for SDL 2 compatibility
{
  int cx = sense_rect.x + sense_rect.sx / 2;
  int cy = sense_rect.y + sense_rect.sy / 2;
  int dx = x - cx;
  int dy = y - cy;
  float angle_radians = atan2f(float(dy), float(dx));
  int degrees_zero_x_axis = int(180.0f / 3.1415926f * angle_radians);
  int degrees_zero_y_axis = degrees_zero_x_axis + 90;
  return -( (degrees_zero_y_axis > 180) ? degrees_zero_y_axis - 360 : degrees_zero_y_axis );
}

void exec_display_list_element( SDL_Renderer *renderer, DRAW_STATE *dstate, CMD_CODE code,
                                std::vector<int> args, std::vector<std::string> sargs )
//----------------------------------------------------------------------------------------
// Set the draw state or draw something.
// Or execute a non-graphical configuration command.
{
  switch( code ){    

  case GRF_clear:                    // r, g, b, a
    SDL_SetRenderDrawColor(renderer, args[0], args[1], args[2], args[3]);
    SDL_RenderClear(renderer);    
    break;
    
  case GRF_rectangle:                // x1, y1, x2, y2, r, g, b, a
    rectangleRGBA(renderer, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
    break;
    
  case GRF_rounded_rectangle:        // x1, y1, x2, y2, rad, r, g, b, a
    roundedRectangleRGBA(renderer, args[0], args[1], args[2], args[3], args[4],
                         args[5], args[6], args[7], args[8]);
    break;
    
  case GRF_rectangle_filled:         // x1, y1, x2, y2, r, g, b, a
    boxRGBA(renderer, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
    break;
    
  case GRF_rounded_rectangle_filled: // x1, y1, x2, y2, rad, r, g, b, a
    roundedBoxRGBA(renderer, args[0], args[1], args[2], args[3], args[4],
                         args[5], args[6], args[7], args[8]);    
    break;
    
  case GRF_line:                     // x1, y1, x2, y2, r, g, b, a
    aalineRGBA(renderer, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
    break;
    
  case GRF_thick_line:               // x1, y1, x2, y2, w, r, g, b, a
    thickLineRGBA(renderer, args[0], args[1], args[2], args[3], args[4],
                  args[5], args[6], args[7], args[8]);
    break;
    
  case GRF_circle:                   // xc, yc, rad, r, g, b, a
    aacircleRGBA(renderer, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);    
    break;
    
  case GRF_circle_filled:            // xc, yc, rad, r, g, b, a
    filledCircleRGBA(renderer, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
    break;
    
  case GRF_arc:                      // xc, yc, r, sa, ea, r, g, b, a
    arcRGBA(renderer, args[0], args[1], args[2], args[3], args[4],
            args[5], args[6], args[7], args[8]);
    break;
    
  case GRF_ellipse:                  // xc, yc, rx, ry, r, g, b, a
    aaellipseRGBA(renderer, args[0], args[1], args[2], args[3],
                  args[4], args[5], args[6], args[7]);
    break;

  case GRF_ellipse_filled:           // xc, yc, rx, ry, r, g, b, a
    filledEllipseRGBA(renderer, args[0], args[1], args[2], args[3],
                      args[4], args[5], args[6], args[7]);
    break;
    
  case GRF_text:                     // x, y, string, centered, r, g, b, a
    draw_text_8x8(renderer, args[4], args[5], args[6], args[7],
                  sargs[1], args[0], args[1], (args[2] != 0), args[3]);
    break;
    
  case GRF_text_in_font:             // x, y, string, centered, r, g, b, ifont
    set_font(*dstate, args[7]);
    if( NULL != dstate->font ){
      draw_text_ttf(renderer, dstate->font, args[4], args[5], args[6],
                    sargs[1], args[0], args[1], (args[2] != 0), args[3]);
    }
    break;

  case GRF_sub_texture:              // name, x, y, downscale, center, rotang, rotx, roty
    draw_sub_texture_from_atlas(renderer, dstate->tex_atlas,
                                sargs[1], args[0], args[1],
                                args[2], (args[3] != 0),
                                args[4], args[5], args[6]);
    break;

  case GRF_show_display_sync:        // <none>
    SDL_RenderPresent(renderer);
    send_sync();
    break;

  case GRF_show_display:             // <none>
    SDL_RenderPresent(renderer);
    break;

  case CFG_load_font:                // filename, size, font number.
    load_TTF_font(sargs[1], args[0], args[1], *dstate);
    send_sync();
    break;

  case CFG_load_texture:             // filename
    if( dstate->tex_atlas.valid )
      clean_up_texture_atlas(dstate->tex_atlas);
    dstate->tex_atlas = load_texture_atlas(renderer, sargs[1]);
    send_sync();
    break;

  case CFG_sense_rect:               // tag, x, y, sx, sy, events
    {
      SENSE_RECT sr;
      sr.x = args[0];
      sr.y = args[1];
      sr.sx = args[2];
      sr.sy = args[3];
      sr.tag = sargs[1];

      // Check if this overlaps any existing sense rect. If it does, that is an error.
      bool overlaps = false;
      int xh_new = sr.x + sr.sx - 1;
      int yh_new = sr.y + sr.sy - 1;
      for( size_t i=0; i<dstate->sense_rects.size(); i++ ){
        int xh_old = dstate->sense_rects[i].x + dstate->sense_rects[i].sx - 1;
        int yh_old = dstate->sense_rects[i].y + dstate->sense_rects[i].sy - 1;
        if( xh_new < dstate->sense_rects[i].x)continue;
        if( sr.x > xh_old )continue;
        if( yh_new < dstate->sense_rects[i].y)continue;
        if( sr.y > yh_old )continue;
        overlaps = true;
        break;
      }

      // Add if not overlapping.
      if( overlaps ){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Sense rectangle tag: %s overlaps an existing rectangle. Not added.\n", sr.tag.c_str());
      }
      else{
        sr.event_bits = 0;
        if( sargs[2].find('d') != std::string::npos )sr.event_bits |= SR_DOWN_EVENT;
        if( sargs[2].find('u') != std::string::npos )sr.event_bits |= SR_UP_EVENT;
        if( sargs[2].find('a') != std::string::npos )sr.event_bits |= SR_ANGLE_EVENT;
        if( sargs[2].find('m') != std::string::npos )sr.event_bits |= SR_MOTION_EVENT;
        if( sargs[2].find('w') != std::string::npos )sr.event_bits |= SR_WHEEL_EVENT;
        sr.x_down = -1;
        sr.y_down = -1;
        dstate->sense_rects.push_back(sr);
      }
    }
    break;

  case CFG_debug:                    // <none>
    dstate->debug_mode = ! dstate->debug_mode;
    break;

  case CFG_reset_display:            // <none>
    break;

  case CFG_use_overlay:              // <none>
    break;
    
  case CFG_use_main:                 // <none>
    break;

  case CFG_remove:                   // x, y (handled in read_thread()).
    break;

  case CFG_NOP:                      // <none>
    break;
  }
}

void draw_display_list( SDL_Renderer *renderer,
                        DISPLAY_LIST* dlist,
                        DRAW_STATE* dstate,
                        CMD_MAP& cmdmap,
                        bool lock_list,
                        bool unlock_list )
//-------------------------------------------------------------------
// Draw all elements in the display list.
{
  bool locked = true;
  if( dstate->debug_mode )
    fprintf(stderr, "draw_display_list(), dlist ptr = %p\n", dlist);
  
  // Get the display list mutex. Optionally.
  if( lock_list ){
    if( SDL_LockMutex(dlist->mutex) < 0 ){
      SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                      "Failed to lock mutex in event handler.\n");
      locked = false;
    }
  }

  // Decode and draw the primitives in the display list.
  if( locked ){
    size_t num_prims = dlist->prims.size();
    if( dstate->debug_mode )
      fprintf(stderr, "... num_prims = %ld\n", num_prims);

    // Iterate over the display list.
    for( int i=0; i<(int)num_prims; i++ ){

      // Get the arguments.
      std::vector<int> args = extract_int_args(dlist->prims[i]);
      std::vector<std::string> sargs = extract_string_args(dlist->prims[i]);
        
      // Identify the command. Ignore unknown commands.
      // There must be at least one string argument: the command.
      if( sargs.size() > 0 ){

        // See if it is a non-graphical command.
        bool non_graphical = ( sargs[0][0] == '*' );

        // Ensure the command is known.
        if( cmdmap.find(sargs[0]) != cmdmap.end() ){
          CMD_INFO cur_cmd = cmdmap[sargs[0]];

          // Check for correct argument counts.
          if( ((int)args.size() != cur_cmd.n_ints) || ((int)sargs.size() != (cur_cmd.n_strings+1)) ){
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Command %s has invalid argument count(s): %ld/%d %ld/%d\n", sargs[0].c_str(),
                         args.size(), cur_cmd.n_ints, sargs.size()-1, cur_cmd.n_strings );
          } // bad arg counts.

          // Execute the graphics command.
          else{
            exec_display_list_element(renderer, dstate, cur_cmd.code, args, sargs);

            // Convert show_display_sync into show_display.
            if( sargs[0] == "*show_display_sync" )
              dlist->prims[i] = "$*show_display$";

            // If non-graphical, turn it into a NOP.
            if( non_graphical )
              dlist->prims[i] = "$*NOP$";
          }
        } // known command.

        else{
          SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                       "Command %s is not defined.\n", sargs[0].c_str() );
        }
      } // at least 1 string arg (command name).
    } // over display list elements.

    // Now remove all "$*NOP$ from the display list vector.
    if( dstate->debug_mode )
      fprintf(stderr, "Before pruning NOPs: num_prims = %ld\n", dlist->prims.size());
    dlist->prims.erase(std::remove_if(dlist->prims.begin(),
                                      dlist->prims.end(),
                                      [](std::string s){return s == "$*NOP$";}),
                       dlist->prims.end());
    if( dstate->debug_mode )
      fprintf(stderr, "... after pruning NOPs: num_prims = %ld\n", dlist->prims.size());

    // Release the display list mutex. Optionally.
    if( unlock_list ){
      if( SDL_UnlockMutex(dlist->mutex) < 0 ){
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to unlock mutex in event handler.\n");
      }
    }
  } // locked mutex OK.
}

bool remove_display_list_element( DRAW_STATE *dstate, CMD_CODE code,
                                  std::vector<int> args, int kx, int ky )
//----------------------------------------------------------------------------------------
// Remove certain display list elements at a specified position.
{
  switch( code ){    
    
  case GRF_rectangle:                // x1, y1, x2, y2, r, g, b, a
  case GRF_rounded_rectangle:        // x1, y1, x2, y2, rad, r, g, b, a
  case GRF_rectangle_filled:         // x1, y1, x2, y2, r, g, b, a
  case GRF_rounded_rectangle_filled: // x1, y1, x2, y2, rad, r, g, b, a
  case GRF_line:                     // x1, y1, x2, y2, r, g, b, a
  case GRF_thick_line:               // x1, y1, x2, y2, w, r, g, b, a
  case GRF_circle:                   // xc, yc, rad, r, g, b, a
  case GRF_circle_filled:            // xc, yc, rad, r, g, b, a
  case GRF_arc:                      // xc, yc, rad, sa, ea, r, g, b, a
  case GRF_ellipse:                  // xc, yc, rx, ry, r, g, b, a
  case GRF_ellipse_filled:           // xc, yc, rx, ry, r, g, b, a
  case GRF_text:                     // x, y, string, centered. dy, r, g, b, a
  case GRF_text_in_font:             // x, y, string, centered, dy, r, g, b, ifont
    if( (args[0] == kx) && (args[1] == ky) )
      return true;
    break;
    
  case GRF_sub_texture:              // name, x, y, downscale, center, rotang, rotx, roty
    if( (args[0] == kx) && (args[1] == ky) )
      return true;
    break;

  default:
    return false;
  }

  return false;
}

void remove_from_display_list( DISPLAY_LIST* dlist,
                               DRAW_STATE* dstate,
                               CMD_MAP& cmdmap,
                               int x, int y )
//-------------------------------------------------------------------
// Remove elements in the display list that are at (x,y).
// The display list is known to be locked on entry.
{
  if( dstate->debug_mode )
    fprintf(stderr, "remove_from_display_list(), dlist ptr = %p\n", dlist);
  
  // Decode and possibly remove some of the primitives in the display list.
  size_t num_prims = dlist->prims.size();
  if( dstate->debug_mode )
    fprintf(stderr, "... num_prims = %ld\n", num_prims);

  // Iterate over the display list.
  for( int i=0; i<(int)num_prims; i++ ){
    if( dstate->debug_mode )
      fprintf(stderr, "[[%s]]\n", dlist->prims[i].c_str());

    // Get the arguments.
    std::vector<int> args = extract_int_args(dlist->prims[i]);
    std::vector<std::string> sargs = extract_string_args(dlist->prims[i]);
        
    // Identify the command. Ignore unknown commands.
    // There must be at least one string argument: the command.
    if( sargs.size() > 0 ){

      // Ensure the command is known.
      if( cmdmap.find(sargs[0]) != cmdmap.end() ){
        CMD_INFO cur_cmd = cmdmap[sargs[0]];

        // Check for correct argument counts.
        if( ((int)args.size() == cur_cmd.n_ints) ){
          if( remove_display_list_element(dstate, cur_cmd.code, args, x, y) ){
            if( dstate->debug_mode )
              fprintf(stderr, "*** Removed at %d,%d (%s)\n", x, y, sargs[0].c_str());
            dlist->prims[i] = "$*NOP$";
          }
        } // correct int arg count.
      } // known command.
    } // at least 1 string arg (command name).
  } // over display list elements.

  // Now remove all "$*NOP$ from the display list vector.
  if( dstate->debug_mode ){
    fprintf(stderr, "... before pruning NOPs (remove): num_prims = %ld\n", dlist->prims.size());
  }
  dlist->prims.erase(std::remove_if(dlist->prims.begin(),
                                    dlist->prims.end(),
                                    [](std::string s){return s == "$*NOP$";}),
                     dlist->prims.end());
  if( dstate->debug_mode ){
    fprintf(stderr, "... after pruning NOPs (remove): num_prims = %ld\n", dlist->prims.size());
    //for( int i=0; i<(int)dlist->prims.size(); i++ ){
    //  fprintf(stderr, "[[%s]]\n", dlist->prims[i].c_str());
    //}
  }
}

int main (int ArgCount, char **Args)
{
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Event event;

  DISPLAY_LIST dlist;  // Main display list.
  DISPLAY_LIST olist;  // Overlay display list.
  DRAW_STATE dstate;   // Graphics state.
  int iw = 1000;       // Default window width.
  int ih = 1000;       // Default window height.
  std::string title = "DSURF";  // Default window title.
  int quit = 0;        // Not quitting yet.
  int x_last_down = -1;
  int y_last_down = -1;

  SDL_version compiled;
  SDL_version linked;
 
  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);
  SDL_Log("Compiled against SDL version %u.%u.%u ...\n",
          compiled.major, compiled.minor, compiled.patch);
  SDL_Log("Linked against SDL version %u.%u.%u.\n",
          linked.major, linked.minor, linked.patch);

  // Initialise the drawing state.
  dstate.debug_mode = false;
  dstate.keep_on_eof = false;
  dstate.tex_atlas.valid = false;
  dstate.fast_update = false;
  dstate.font = NULL;
  for( int i=0; i<MXF; i++ )
    dstate.fonts[i] = NULL;
  dstate.cur_font = 0;
  dstate.fontdir = "/usr/share/fonts/truetype/liberation2";

  // Command map.
  CMD_MAP cmdmap
    {
      {"clear", {GRF_clear, 4, 0}},
      {"rectangle", {GRF_rectangle, 8, 0}},
      {"rounded_rectangle", {GRF_rounded_rectangle, 9, 0}},
      {"rectangle_filled", {GRF_rectangle_filled, 8, 0}},
      {"rounded_rectangle_filled", {GRF_rounded_rectangle_filled, 9, 0}},
      {"line", {GRF_line, 8, 0}},
      {"thick_line", {GRF_thick_line, 9, 0}},
      {"circle", {GRF_circle, 7, 0}},
      {"circle_filled", {GRF_circle_filled, 7, 0}},
      {"arc", {GRF_arc, 9, 0}},
      {"ellipse", {GRF_ellipse, 8, 0}},
      {"ellipse_filled", {GRF_ellipse_filled, 8, 0}},
      {"text", {GRF_text, 8, 1}},
      {"text_in_font", {GRF_text_in_font, 8, 1}},

      {"sub_texture", {GRF_sub_texture, 7, 1}},

      {"*show_display_sync", {GRF_show_display_sync, 0, 0}},
      {"*show_display", {GRF_show_display, 0, 0}},

      {"*reset_display", {CFG_reset_display, 0, 0}},
      {"*use_overlay", {CFG_use_overlay, 0, 0}},
      {"*use_main", {CFG_use_main, 0, 0}},
      {"*load_font", {CFG_load_font, 2, 1}},
      {"*load_texture", {CFG_load_texture, 0, 1}},
      {"*sense_rect", {CFG_sense_rect, 4, 2}},
      {"*remove", {CFG_remove, 2, 0}},
      {"*debug", {CFG_debug, 0, 0}},
      {"*NOP", {CFG_NOP, 0, 0}}
    };
  dstate.cmdmap = cmdmap;

  // Parse arguments.
  CLI::App app{"Display surface program for a68g"};
  Args = app.ensure_utf8(Args);

  app.add_option("-x,--width", iw, "Width of display window in pixels.");
  app.add_option("-y,--height", ih, "Height of display window in lines.");
  app.add_option("-t,--title", title, "Set the window title.");
  app.add_option("-f,--fontdir", dstate.fontdir, "TrueType fonts directory to use.");

  app.add_flag("-d,--debug", dstate.debug_mode, "Turn on debug output.");
  app.add_flag("-k,--keep", dstate.keep_on_eof, "Keep open after EOF on stdin.");
  app.add_flag("-u,--update", dstate.fast_update, "Update SDL immediately. Don't wait for show_display.");

  CLI11_PARSE(app, ArgCount, Args);

  // Setup SDL.
  if( SDL_Init(SDL_INIT_VIDEO) != 0 ){
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Unable to initialize SDL: %s", SDL_GetError());
    return 1;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"best");
  if (SDL_CreateWindowAndRenderer(iw, ih, 0, &window, &renderer)) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Couldn't create window and renderer: %s", SDL_GetError());
    return 1;
  }
  if( TTF_Init() != 0 ){
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Unable to initialize TTF: %s", SDL_GetError());
    return 1;    
  }
  SDL_SetWindowBordered(window, SDL_TRUE ); //FALSE);
  SDL_SetWindowTitle(window, title.c_str());
  SDL_RaiseWindow(window);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  // Load a default font for TTF text into font 0.
  if( load_TTF_font("LiberationSans-Regular.ttf", 9, 0, dstate) != 0 ){
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Couldn't open default TTF font: %s", SDL_GetError());
    return 1;    
  }
  set_font(dstate, 0);

  // Create a mutex to protect the display list(s). The "main" and "overlay"
  // display lists are both protected by the single mutex (in the "dlist" display list).
  // They are always drawn one immediately after the other.
  dlist.mutex = SDL_CreateMutex();
  if( NULL == dlist.mutex ) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Couldn't create display list mutex: %s", SDL_GetError());
    return 1;
  }
  olist.mutex = dlist.mutex;
  dlist.prims.clear();
  olist.prims.clear();

  // Start read thread.
  THREAD_DATA thread_data;
  thread_data.dstate = &dstate;
  thread_data.dlist = &dlist;
  thread_data.olist = &olist;
  SDL_Thread* reader_thread = SDL_CreateThread(read_thread, "reader", (void*)(&thread_data));
  if( NULL == reader_thread ){
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Unable to start reader thread: %s", SDL_GetError());
    return 1;
  }
  SDL_DetachThread(reader_thread);

  // Loop forever ...
  while(1){

    // Wait for some event, not consuming CPU cycles.
    SDL_WaitEvent(&event);

    //fprintf(stderr,"Here\n");
    bool drawn = false;
    
    // Process all events.                 .
    while(1){
      //fprintf(stderr, "event\n");

      // Quit event:  exit the program.
      if( event.type == SDL_QUIT ){
        quit = 1;
        drawn = true; // Don't bother redrawing if exiting.
        break;
      }

      // Keyboard presses ...
      if( event.type == SDL_KEYDOWN ){

        // q: exit the program.
        if(event.key.keysym.sym == SDLK_q){
          quit = 1;
          drawn = true; // Don't bother redrawing if exiting.
          break;
        }

      } // key press

      // Mouse interactions ...
      if( (event.type == SDL_MOUSEBUTTONDOWN) || (event.type == SDL_MOUSEBUTTONUP) ){
        std::string tag;
        int x = event.button.x;
        int y = event.button.y;
        if( event.type == SDL_MOUSEBUTTONDOWN ){
          x_last_down = x;
          y_last_down = y;
        }
        int sr_index = in_sense_rect(x, y, dstate.sense_rects,
                                     (event.type == SDL_MOUSEBUTTONDOWN) ? SR_DOWN_EVENT : SR_UP_EVENT,
                                     tag);
        if( sr_index >= 0 ){
          printf("%s,%s,%d,%d\n", tag.c_str(), (event.type == SDL_MOUSEBUTTONDOWN) ? "D" : "U", x, y);
          fflush(stdout);
        }
      } // Mouse button down/up.

      if( event.type == SDL_MOUSEMOTION ){
        std::string tag;
        int x = event.motion.x;
        int y = event.motion.y;
        int sr_index = in_sense_rect(x, y, dstate.sense_rects,
                                     (SR_MOTION_EVENT | SR_ANGLE_EVENT),
                                     tag);
        if( sr_index >= 0 ){

          // Interested in all forms of motion inside the rect?
          if( (dstate.sense_rects[sr_index].event_bits & SR_MOTION_EVENT) != 0 ){
            printf("%s,M,%d,%d\n", tag.c_str(), x, y);
            fflush(stdout);
          }

          // Interested in angle then. Calculate it.
          else{
            int angle = sense_rect_angle(dstate.sense_rects[sr_index], x, y);
            //fprintf(stderr,"Motion, sr_index=%d, %s,A,%d\n",sr_index, tag.c_str(), angle);
            printf("%s,A,%d\n", tag.c_str(), angle);
            fflush(stdout);
          }
        }
      } // Mouse motion.   

      if( event.type == SDL_MOUSEWHEEL ){
        std::string tag;
        int sr_index = in_sense_rect(x_last_down, y_last_down, dstate.sense_rects,
                                     SR_WHEEL_EVENT,
                                     tag);
        if( sr_index >= 0 ){
          printf("%s,W,%d\n", tag.c_str(), event.wheel.y);
          fflush(stdout);
        }
      } // Mouse wheel moved.

      // User event sent when display list has been modified.
      // At this point, there may be non-graphical commands as well as things to draw.
      if (event.type == SDL_USEREVENT) {

        // Redraw. I.e. process the display list, which may not update what is seen,
        // unless fast_update. Always draw the "main" display list items, then the
        // "overlay" list items (on top).
        draw_display_list(renderer, &dlist, &dstate, cmdmap, true, false);
        draw_display_list(renderer, &olist, &dstate, cmdmap, false, true);
        if( dstate.fast_update )
          SDL_RenderPresent(renderer);
        drawn = true;
      } // user event

      // If there is another event, get it. Otherwise, exit event handling loop.
      if( ! SDL_PollEvent(&event) )
        break;        

    } // process events loop.

    // If quitting, exit the run loop.
    if( quit )
      break;

    // If we haven't already "drawn" the list, do it now.
    if( ! drawn ){
      draw_display_list(renderer, &dlist, &dstate, cmdmap, true, false);
      draw_display_list(renderer, &olist, &dstate, cmdmap, false, true);
      SDL_RenderPresent(renderer);
    }
 
  } // run loop.

  // Clean up SDL.
  if( dstate.debug_mode ){
    size_t num_prims = dlist.prims.size();
    fprintf(stderr, "FINAL num_prims = %ld\n", num_prims);

    // Iterate over the display list.
    for( int i=0; i<(int)num_prims; i++ ){
      fprintf(stderr, "[%s]\n", dlist.prims[i].c_str());
    }

    size_t overlay_num_prims = olist.prims.size();
    fprintf(stderr, "FINAL overlay_num_prims = %ld\n", overlay_num_prims);

    // Iterate over the display list.
    for( int i=0; i<(int)overlay_num_prims; i++ ){
      fprintf(stderr, "[%s]\n", olist.prims[i].c_str());
    }
  }
  
  clean_up_texture_atlas(dstate.tex_atlas);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  for( int i=0; i<MXF; i++ ){
    if( NULL != dstate.fonts[i] )
      TTF_CloseFont(dstate.fonts[i]);
  }
  TTF_Quit();
  SDL_Quit();
  
  return 0;
}
