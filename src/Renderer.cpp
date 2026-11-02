// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "Renderer.h"
#include "BlenderMeshes.h"
#include "Tga.h"
#include "OSGraphics.h"
#include "UiAppearance.h"
#include "NoteEmitter.h"
#include "ImpactFlameEmitter.h"
#include "StageScenes.h"
#include "TopBirthEmitter.h"

//#include <GL/glx.h>
//#include <GL/glext.h>
//#include <stdio.h>

#include <limits>
#include <cmath>
#include <algorithm>
#include <vector>
#include <GL/glext.h>
#include <cstdio>

using namespace std;

// These are static because OpenGL is (essentially) static
static unsigned int last_texture_id = numeric_limits<unsigned int>::max();

// Registrar el puntero para sombrear solamente el fondo real de los
// botones dibujados con rectángulos. No dibujar superposiciones: provocaban
// recuadros que sobresalían de los botones y diálogos.
namespace {
struct PressState {
  int x=0,y=0;
  bool held=false;
};
PressState pressState;
}

void SelectTexture(unsigned int texture_id) {

  if (texture_id == last_texture_id)
    return;

  glBindTexture(GL_TEXTURE_2D, texture_id);
  last_texture_id = texture_id;
}


Renderer::Renderer(SDL_Window* sdl_window) :
  m_xoffset(0),
  m_yoffset(0),
  m_sdl_window(sdl_window) {
}

SDL_Color Renderer::ToColor(int r, int g, int b, int a) {

  SDL_Color c;
  c.r = r;
  c.g = g;
  c.b = b;
  c.a = a;

  return c;
}

void Renderer::SetVSyncInterval(int interval) {
// #ifdef WIN32

//    const char *extensions = reinterpret_cast<const char*>(static_cast<const unsigned char*>(glGetString( GL_EXTENSIONS )));

//    // Check if the WGL_EXT_swap_control extension is supported.
//    if (strstr(extensions, "WGL_EXT_swap_control") == 0) return;

//    typedef BOOL (APIENTRY *SWAP_INTERVAL_PROC)( int );
//    SWAP_INTERVAL_PROC wglSwapIntervalEXT = (SWAP_INTERVAL_PROC)wglGetProcAddress( "wglSwapIntervalEXT" );
//    if (wglSwapIntervalEXT) wglSwapIntervalEXT(interval);

// #else

//    GLint i = interval;
//    GLboolean ret = aglSetInteger(m_glcontext, AGL_SWAP_INTERVAL, &i);
//    if (ret == GL_FALSE)
//    {
//       // LOGTODO!
//       // This is non-critical.  V-Sync might just not be supported.
//    }

// #endif

// X
/*
  if (!m_vsync_initialized) {
    m_vsync_initialized = true;
    Display * x11_display = XOpenDisplay(0);

    static PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = NULL;
    static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalMESA = NULL;
    static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = NULL;

    string extensions = glXQueryExtensionsString(x11_display, DefaultScreen(x11_display));
    if (extensions.find("GLX_EXT_swap_control") != -1) {
      glXSwapIntervalEXT  = (PFNGLXSWAPINTERVALEXTPROC) glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");
      cout << "GLX_EXT_swap_control disponible\n";
    }
    if (extensions.find("GLX_MESA_swap_control") != -1) {
      glXSwapIntervalMESA = (PFNGLXSWAPINTERVALSGIPROC) glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalMESA");
      cout << "GLX_MESA_swap_control disponible\n";
    }
    if (extensions.find("GLX_SGI_swap_control") != -1) {
      glXSwapIntervalSGI  = (PFNGLXSWAPINTERVALSGIPROC) glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalSGI");
      cout << "GLX_SGI_swap_control disponible\n";
    }

    int val = interval != 0 ? 1 : 0;
    if (glXSwapIntervalMESA) {
      glXSwapIntervalSGI(val);
      cout << "glXSwapIntervalSGI ajustado a " << val << "\n";
    }
    if (glXSwapIntervalSGI) {
      glXSwapIntervalMESA(val);
      cout << "glXSwapIntervalMESA ajustado a " << val << "\n";
    }
    if (glXSwapIntervalEXT) {
      GLXDrawable drawable = glXGetCurrentDrawable();
      glXSwapIntervalEXT(x11_display, drawable, val);
      cout << "glXSwapIntervalEXT ajustado a " << val << "\n";
    }
  }
  */
}

void Renderer::SwapBuffers() {
  SDL_GL_SwapWindow(m_sdl_window);
}

void Renderer::ForceTexture(unsigned int texture_id) {
  last_texture_id = numeric_limits<unsigned int>::max();
  SelectTexture(texture_id);
}

void Renderer::BeginPressFrame(int x,int y,bool leftHeld){
  pressState.x=x;
  pressState.y=y;
  pressState.held=leftHeld;
}

void Renderer::DrawPressFeedback(){
  // No-op deliberado: no dibujar rectángulos adicionales sobre la UI.
  // La pulsación se representa al dibujar cada botón, dentro de su figura.
}

void Renderer::BeginPianoPerspective(float centerX,float topY,float bottomY,
                                     int perspectiveStyle,int depth,int angle,
                                     int horizon,int stageWidth){
  // Transformación homográfica vinculada al teclado: las cuatro
  // esquinas del borde inferior quedan exactamente en su posición 2D.
  // El motor anterior desplazaba el borde inferior y hacía flotar notas.
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  const float height=std::max(1.0f,bottomY-topY);
  const float perspective=(perspectiveStyle==1?1.16f:0.86f);
  const float tilt=0.82f+0.24f*
                   std::max(0,std::min(65,angle))/65.0f;
  const float widthFactor=100.0f/
                   std::max(40,std::min(120,stageWidth));
  float k=(0.00016f+0.00108f*
          std::max(0,std::min(100,depth))/100.0f)
          *perspective*tilt*widthFactor;
  // Nunca permitir el efecto embudo: el borde superior conserva
  // al menos el 70 % del ancho del teclado.
  k=std::min(k,0.42f/height);
  // El horizonte es un punto de fuga gráfico, no un recorte del teclado.
  const float horizonY=topY+height*
                      std::max(10,std::min(60,horizon))/100.0f;
  const float C=(bottomY-horizonY)*(1.0f+k*height)/height;
  const GLfloat matrix[16]={
    1.0f,0.0f,0.0f,0.0f,
    -centerX*k,C-bottomY*k,0.0f,-k,
    0.0f,0.0f,1.0f,0.0f,
    centerX*k*bottomY,bottomY+(bottomY*k-C)*bottomY,0.0f,
    1.0f+k*bottomY
  };
  glMultMatrixf(matrix);
}

void Renderer::BeginPianoRollClip(int x,int top,int right,int bottom){
  m_scissor_was_enabled=glIsEnabled(GL_SCISSOR_TEST)==GL_TRUE;
  glGetIntegerv(GL_SCISSOR_BOX,m_old_scissor);
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT,viewport);
  int drawableW=0,drawableH=0;
  SDL_GL_GetDrawableSize(m_sdl_window,&drawableW,&drawableH);
  if(drawableW<1 || drawableH<1)return;
  const float logicalW=static_cast<float>(
      UiAppearance::LogicalWidth(drawableW,drawableH));
  const float logicalH=static_cast<float>(
      UiAppearance::LogicalHeight(drawableW,drawableH));
  const float sx=viewport[2]/std::max(1.0f,logicalW);
  const float sy=viewport[3]/std::max(1.0f,logicalH);
  const int l=static_cast<int>(std::floor((x+m_xoffset)*sx));
  const int r=static_cast<int>(std::ceil((right+m_xoffset)*sx));
  const int t=static_cast<int>(std::floor((top+m_yoffset)*sy));
  const int b=static_cast<int>(std::ceil((bottom+m_yoffset)*sy));
  glEnable(GL_SCISSOR_TEST);
  glScissor(std::max(0,l),std::max(0,viewport[3]-b),
            std::max(1,std::min(viewport[2],r)-std::max(0,l)),
            std::max(1,std::min(viewport[3],b)-std::max(0,t)));
}

void Renderer::EndPianoRollClip(){
  glScissor(m_old_scissor[0],m_old_scissor[1],
            m_old_scissor[2],m_old_scissor[3]);
  if(!m_scissor_was_enabled)glDisable(GL_SCISSOR_TEST);
}

void Renderer::EndPianoPerspective(){
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

void Renderer::SetColor(SDL_Color c) {
  SetColor(c.r, c.g, c.b, c.a);
}

void Renderer::SetColor(int r, int g, int b, int a) {
  UiAppearance::Transform(r,g,b);
  glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

void Renderer::DrawRoundedGradient(int x,int y,int w,int h,int radius,
                                   SDL_Color top,SDL_Color bottom){
  if(w<=0 || h<=0)return;
  // Dibujamos geometría real redondeada, no rectángulos con halos cuadrados.
  // No se necesitan texturas externas ni shaders.
  SelectTexture(0);
  const int rad=std::max(0,std::min(radius,std::min(w,h)/2));
  const auto shade=[&](float relativeY){
    const float f=std::max(0.0f,std::min(1.0f,relativeY));
    const auto channel=[&](int a,int b)->int{
      return static_cast<int>(a+(b-a)*f+0.5f);
    };
    SetColor(channel(top.r,bottom.r),channel(top.g,bottom.g),
             channel(top.b,bottom.b),channel(top.a,bottom.a));
  };
  if(rad==0){
    glBegin(GL_QUADS);
    shade(0);glVertex3f(x+m_xoffset,y+m_yoffset,0);
    shade(0);glVertex3f(x+w+m_xoffset,y+m_yoffset,0);
    shade(1);glVertex3f(x+w+m_xoffset,y+h+m_yoffset,0);
    shade(1);glVertex3f(x+m_xoffset,y+h+m_yoffset,0);
    glEnd();
    return;
  }
  constexpr float PI=3.14159265358979323846f;
  const int segments=6;
  glBegin(GL_TRIANGLE_FAN);
  shade(.5f);
  glVertex3f(x+w*.5f+m_xoffset,y+h*.5f+m_yoffset,0);
  const auto arc=[&](float cx,float cy,float start){
    for(int i=0;i<=segments;i++){
      const float a=start+(PI/2)*i/segments;
      const float px=cx+rad*std::cos(a);
      const float py=cy+rad*std::sin(a);
      shade((py-y)/h);
      glVertex3f(px+m_xoffset,py+m_yoffset,0);
    }
  };
  arc(x+rad,y+rad,PI);
  arc(x+w-rad,y+rad,PI*1.5f);
  arc(x+w-rad,y+h-rad,0);
  arc(x+rad,y+h-rad,PI*.5f);
  // Cerrar el abanico: sin este último triángulo, las notas largas
  // presentan un corte diagonal por todo su lateral izquierdo.
  shade(static_cast<float>(rad)/h);
  glVertex3f(x+m_xoffset,y+rad+m_yoffset,0);
  glEnd();
}



void Renderer::BeginBlenderKeyboard3D(){
  ForceTexture(0);
  // La superficie superior de cada tecla ocupa profundidad física real.
  // Limpiamos el depth buffer exclusivamente antes de este subescenario.
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_TRUE);
}
void Renderer::EndBlenderKeyboard3D(){
  glDisable(GL_DEPTH_TEST);
}
void Renderer::BeginBlenderNotes3D(){
  ForceTexture(0);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_TRUE);
}
void Renderer::EndBlenderNotes3D(){
  glDisable(GL_DEPTH_TEST);
}

namespace {
struct PianoKeyPoint { GLfloat u,t,z; };
struct PianoKeyColor { GLubyte r,g,b,a; };
struct PianoKeySurface {
  std::vector<PianoKeyPoint> points;
  std::vector<PianoKeyColor> material;
  std::vector<PianoKeyColor> themed[3];

  explicit PianoKeySurface(bool black){
    const float scaleX=black?.62f:.92f;
    const float scaleY=black?.64f:.98f;
    const float perspective=black?.033f:.044f;
    const float materialR=black?34.0f:238.0f;
    const float materialG=black?39.0f:239.0f;
    const float materialB=black?46.0f:242.0f;
    const BlenderMesh::Vertex* vertices=black?BlenderMesh::Black:
                                              BlenderMesh::White;
    const size_t count=black?BlenderMesh::BlackCount:BlenderMesh::WhiteCount;
    points.reserve(count);material.reserve(count);
    for(size_t i=0;i<count;++i){
      const auto& v=vertices[i];
      const float t=std::max(0.0f,std::min(1.0f,v.y/scaleY));
      points.push_back({(v.x/scaleX)*(1.0f+perspective*t),t,v.z});
      const float diffuse=std::max(0.0f,
            -.26f*v.nx-.40f*v.ny+.88f*v.nz);
      const float spec=std::pow(std::max(0.0f,
            -.13f*v.nx-.19f*v.ny+.97f*v.nz),22.0f);
      const float bevel=std::max(0.0f,1.0f-v.nz);
      const float shade=(.40f+.52f*diffuse+.16f*spec)*
                        (1.0f-.16f*bevel);
      material.push_back({
        static_cast<GLubyte>(std::min(255.0f,materialR*shade)),
        static_cast<GLubyte>(std::min(255.0f,materialG*shade)),
        static_cast<GLubyte>(std::min(255.0f,materialB*shade)),255});
    }
  }
  const std::vector<PianoKeyColor>& colors(){
    auto& result=themed[std::max(0,std::min(2,UiAppearance::Theme()))];
    if(result.empty()){
      result.reserve(material.size());
      for(const auto& c:material){
        int r=c.r,g=c.g,b=c.b;
        UiAppearance::Transform(r,g,b);
        result.push_back({static_cast<GLubyte>(r),
                          static_cast<GLubyte>(g),
                          static_cast<GLubyte>(b),255});
      }
    }
    return result;
  }
};
PianoKeySurface& pianoKeySurface(bool black){
  static PianoKeySurface white(false),dark(true);
  return black?dark:white;
}
} // namespace

void Renderer::DrawPianoKey3D(int x,int y,int w,int h,bool black,
                              float press,bool cue,SDL_Color accent,
                              float keyboardCenter){
  if(w<3 || h<8)return;
  ForceTexture(0);
  const float depression=std::max(0.0f,std::min(1.0f,press));
  const float scaleX=black?.62f:.92f;
  const float scaleY=black?.64f:.98f;
  const BlenderMesh::Vertex* vertices=black?BlenderMesh::Black:
                                            BlenderMesh::White;
  const size_t count=black?BlenderMesh::BlackCount:BlenderMesh::WhiteCount;

  // Una pulsación NO reemplaza el material del piano con color opaco.
  // Primero dibujamos el marfil/grafito, conservando el relieve físico.
  // Después se deposita una lámina translúcida sobre la misma superficie.
  const float zScale=black?56.0f:47.0f;
  const float frontTravel=black?4.8f:6.2f;
  const auto place=[&](const BlenderMesh::Vertex& v){
    const float t=std::max(0.0f,std::min(1.0f,v.y/scaleY));
    const float centeredX=x+w*(v.x/scaleX+.5f);
    const float perspective=1.0f+(black?.033f:.044f)*t;
    const float screenX=keyboardCenter+(centeredX-keyboardCenter)*perspective;
    // La tecla gira alrededor de la zona trasera. El extremo frontal
    // desciende progresivamente; la geometría y los biseles lo acompañan.
    const float drop=frontTravel*depression*t;
    const float screenY=y+t*h-v.z*zScale+drop;
    const float depth=v.z*.65f+(black?.09f:0.0f);
    glVertex3f(screenX+m_xoffset,screenY+m_yoffset,depth);
  };
  // Normales e iluminación del material son constantes: se calculan una
  // sola vez. La matriz aplica tamaño, perspectiva y recorrido de la tecla
  // a los mismos triángulos originales sin rehacer miles de vértices.
  auto& surface=pianoKeySurface(black);
  const auto& colors=surface.colors();
  const float k=black?.033f:.044f;
  const GLfloat transform[16]={
    float(w),0,0,0,
    (x+w*.5f-keyboardCenter)*k,h+frontTravel*depression,0,0,
    0,-zScale,.65f,0,
    x+w*.5f+m_xoffset,float(y+m_yoffset),black?.09f:0.0f,1
  };
  glPushMatrix();
  glMultMatrixf(transform);
  glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glVertexPointer(3,GL_FLOAT,sizeof(PianoKeyPoint),surface.points.data());
  glColorPointer(4,GL_UNSIGNED_BYTE,sizeof(PianoKeyColor),colors.data());
  glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(surface.points.size()));
  glPopClientAttrib();
  glPopMatrix();
  // glDrawArrays deja el color actual indeterminado al usar COLOR_ARRAY.
  const auto& last=colors.back();
  glColor4ub(last.r,last.g,last.b,last.a);

  if(depression>.005f){
    // Capa de contacto con alfa: el color de la nota se percibe, pero
    // permite ver las caras biseladas, los brillos y el recorrido real.
    // Sobre el plano superior, no sobre la cara frontal del piano.
    glDepthMask(GL_FALSE);
    glBegin(GL_TRIANGLES);
    for(size_t i=0;i<count;i+=3){
      const auto& a=vertices[i];
      const auto& b=vertices[i+1];
      const auto& c=vertices[i+2];
      const float nz=(a.nz+b.nz+c.nz)/3.0f;
      if(nz<.68f)continue;
      const float brightness=.82f+.18f*std::max(0.0f,nz);
      SetColor(static_cast<int>(accent.r*brightness),
               static_cast<int>(accent.g*brightness),
               static_cast<int>(accent.b*brightness),
               static_cast<int>((black?93.0f:76.0f)*depression));
      place(a);place(b);place(c);
    }
    glEnd();
    glDepthMask(GL_TRUE);
  }
  // Una guía MIDI indica dónde tocar, pero no hunde ni tiñe la tecla.
  if(cue && depression<=.005f){
    glDisable(GL_DEPTH_TEST);
    SetColor(accent.r,accent.g,accent.b,115);
    DrawQuad(x+1,y-2,std::max(1,w-2),2);
    glEnable(GL_DEPTH_TEST);
  }
}


// Película de fuego orgánico propia: el ruido y la distorsión se calculan
// POR PÍXEL sobre cada nota, no como líneas rectas ni como vórtice externo.
// Se inicializa una vez por contexto; fallback geométrico si falla GLSL.
namespace {
struct NoteFlameFilm {
  GLuint program=0;
  bool attempted=false;
  GLint timeLocation=-1,contactLocation=-1,bloomLocation=-1;
  GLint birthLocation=-1,birthAgeLocation=-1;
  GLint fuelLocation=-1,widthLocation=-1,originLocation=-1;
  GLint extentLocation=-1,tintLocation=-1;
  PFNGLCREATESHADERPROC createShader=nullptr;
  PFNGLSHADERSOURCEPROC shaderSource=nullptr;
  PFNGLCOMPILESHADERPROC compileShader=nullptr;
  PFNGLGETSHADERIVPROC getShaderiv=nullptr;
  PFNGLGETSHADERINFOLOGPROC getShaderInfoLog=nullptr;
  PFNGLCREATEPROGRAMPROC createProgram=nullptr;
  PFNGLATTACHSHADERPROC attachShader=nullptr;
  PFNGLLINKPROGRAMPROC linkProgram=nullptr;
  PFNGLGETPROGRAMIVPROC getProgramiv=nullptr;
  PFNGLGETPROGRAMINFOLOGPROC getProgramInfoLog=nullptr;
  PFNGLDELETESHADERPROC deleteShader=nullptr;
  PFNGLDELETEPROGRAMPROC deleteProgram=nullptr;
  PFNGLUSEPROGRAMPROC useProgram=nullptr;
  PFNGLGETUNIFORMLOCATIONPROC getUniformLocation=nullptr;
  PFNGLUNIFORM1FPROC uniform1f=nullptr;
  PFNGLUNIFORM3FPROC uniform3f=nullptr;
  PFNGLUNIFORM2FPROC uniform2f=nullptr;

  void init(){
    if(attempted)return;
    attempted=true;
    const auto addr=[](const char* s){return SDL_GL_GetProcAddress(s);};
#define GLPROC(member,type,name) member=reinterpret_cast<type>(addr(name));if(!member)return
    GLPROC(createShader,PFNGLCREATESHADERPROC,"glCreateShader");
    GLPROC(shaderSource,PFNGLSHADERSOURCEPROC,"glShaderSource");
    GLPROC(compileShader,PFNGLCOMPILESHADERPROC,"glCompileShader");
    GLPROC(getShaderiv,PFNGLGETSHADERIVPROC,"glGetShaderiv");
    GLPROC(getShaderInfoLog,PFNGLGETSHADERINFOLOGPROC,"glGetShaderInfoLog");
    GLPROC(createProgram,PFNGLCREATEPROGRAMPROC,"glCreateProgram");
    GLPROC(attachShader,PFNGLATTACHSHADERPROC,"glAttachShader");
    GLPROC(linkProgram,PFNGLLINKPROGRAMPROC,"glLinkProgram");
    GLPROC(getProgramiv,PFNGLGETPROGRAMIVPROC,"glGetProgramiv");
    GLPROC(getProgramInfoLog,PFNGLGETPROGRAMINFOLOGPROC,"glGetProgramInfoLog");
    GLPROC(deleteShader,PFNGLDELETESHADERPROC,"glDeleteShader");
    GLPROC(deleteProgram,PFNGLDELETEPROGRAMPROC,"glDeleteProgram");
    GLPROC(useProgram,PFNGLUSEPROGRAMPROC,"glUseProgram");
    GLPROC(getUniformLocation,PFNGLGETUNIFORMLOCATIONPROC,"glGetUniformLocation");
    GLPROC(uniform1f,PFNGLUNIFORM1FPROC,"glUniform1f");
    GLPROC(uniform3f,PFNGLUNIFORM3FPROC,"glUniform3f");
    GLPROC(uniform2f,PFNGLUNIFORM2FPROC,"glUniform2f");
#undef GLPROC
    const char* vert=R"GLSL(
      #version 120
      varying vec2 vUv;
      void main(){
        vUv=gl_MultiTexCoord0.xy;
        gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
      }
    )GLSL";
    const char* frag=R"GLSL(
      #version 120
      varying vec2 vUv;
      uniform vec2 uOrigin;
      uniform vec2 uExtent;
      uniform vec3 uTint;
      uniform float uTime;
      uniform float uFuel;
      uniform float uWidth;
      uniform float uContact;
      uniform float uBloom;
      uniform float uBirth;
      uniform float uBirthAge;
      // Valor suave, estable y continuo para causticas turbulentas.
      float rand2(vec2 p){
        return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);
      }
      float noise2(vec2 p){
        vec2 i=floor(p);
        vec2 f=fract(p);
        f=f*f*f*(f*(f*6.0-15.0)+10.0);
        return mix(mix(rand2(i),rand2(i+vec2(1.0,0.0)),f.x),
                   mix(rand2(i+vec2(0.0,1.0)),rand2(i+vec2(1.0,1.0)),f.x),f.y);
      }
      float fbm(vec2 p){
        float n=0.0;
        n+=.56*noise2(p);
        n+=.28*noise2(p*2.07+vec2(13.4,2.9));
        n+=.16*noise2(p*4.13+vec2(3.7,19.1));
        return n;
      }
      void main(){
        // Coordenadas reales de fragmento: la implementación fija de
        // OpenGL no siempre rellena correctamente gl_MultiTexCoord0.
        vec2 uv=vec2((gl_FragCoord.x-uOrigin.x)/uExtent.x,
             1.0-(gl_FragCoord.y-uOrigin.y)/uExtent.y);
        // Calmer, flowing particles: animation time changes only the
        // visuals, never the NoteOn/NoteOff duration or piano timing.
        float t=uTime*.62;
        // Bloom analítico de bajo coste, de bordes suaves, no un rectángulo.
        // Se dibuja ANTES de la llama con poca energía aditiva.
        // No usa framebuffer ni recursos de SeeMusic.
        if(uBloom>.5){
          float distance=abs(uv.x-.5);
          float height=1.0-uv.y;
          float radius=uContact>.5 ? mix(.34,.085,height) : .62;
          float glowSoft=exp(-pow(distance/max(.01,radius),2.0));
          if(uContact>.5){
            if(uBirthAge<0.0||uBirthAge>=1.10){
              gl_FragColor=vec4(0.0);return;
            }
            glowSoft*=1.0-smoothstep(.15,1.10,uBirthAge);
          }
          float taper=uContact>.5 ?
              (1.0-smoothstep(.75,1.0,height)) :
              (smoothstep(.0,.09,uv.y)*
               (1.0-smoothstep(.91,1.0,uv.y)));
          float shimmer=.92+.08*sin(t*2.8+height*8.0);
          gl_FragColor=vec4(uTint,clamp(.105*uFuel*glowSoft*taper*shimmer,0.0,.12));
          return;
        }
        // Real contact ignition. The particle source is a MIDI NoteOn,
        // NOT a candle attached to a sustained key. Every tongue travels
        // vertically and disappears in less than 320 ms.
        if(uContact>.5){
          float age=uBirthAge;
          if(age<0.0 || age>=1.10){gl_FragColor=vec4(0.0);return;}
          float rise=(1.0-uv.y)*uExtent.y;
          float front=age*375.0;  // climbs 300+ screen pixels before vanishing
          float travel=exp(-pow((rise-front+15.0)/54.0,2.0));
          float lower=smoothstep(-2.0,6.0,rise);
          float tail=1.0-smoothstep(.62,1.10,age);
          float curl=fbm(vec2(uv.x*6.5+uTime*1.3,
                               rise*.065-uTime*10.8));
          float swirl=(curl-.5)*.21+
                      .065*sin(rise*.17-age*25.0);
          float width=mix(.22,.029,clamp(rise/105.0,0.0,1.0));
          float dx=uv.x-.5-swirl;
          float mainTongue=exp(-pow(dx/width,2.0));
          float leftTongue=exp(-pow((dx+.15*sin(rise*.108-age*28.0))/.074,2.0));
          float rightTongue=exp(-pow((dx-.14*cos(rise*.091-age*24.0))/.071,2.0));
          float threads=.70*mainTongue+.22*leftTongue+.19*rightTongue;
          float hetero=.48+.52*fbm(vec2(uv.x*13.0,
                                        rise*.091-age*16.0));
          float heat=threads*hetero*travel*lower*tail*uFuel*1.25;
          float core=clamp(mainTongue*.72*heat,0.0,1.0);
          vec3 edgeColor=mix(uTint,vec3(.08,.71,.98),.69);
          vec3 middle=mix(uTint,vec3(1.0,.51,.11),.78);
          vec3 inner=vec3(1.0,.95,.76);
          vec3 flameColor=mix(edgeColor,middle,clamp(heat*1.8,0.0,1.0));
          flameColor=mix(flameColor,inner,clamp(core*.68,0.0,.68));
          gl_FragColor=vec4(flameColor,clamp(heat*.80,0.0,.60));
          return;
        }
        float edge=abs(uv.x-.5)*2.0;
        // Escala física desde el extremo inferior: el dibujo no se comprime
        // ni cambia de velocidad cuando se acorta una nota sostenida.
        // Birth mode reverses the flow's spatial origin: hot plumes
        // propagate downward FROM THE TOP, not upward from the keyboard.
        float flow=(uBirth>.5?uv.y:(uv.y-1.0))*
                   uExtent.y/max(96.0,uWidth*10.0);
        float pixelY=uv.y*uExtent.y;
        float mask=smoothstep(.0,9.0,pixelY)*
                   (1.0-smoothstep(uExtent.y-9.0,uExtent.y,pixelY));
        vec2 advect=vec2(uv.x*3.8+sin(flow*11.8-t*1.8)*.23,
                         flow*8.8-t*1.75);
        float warp=fbm(advect);
        float fold=fbm(vec2(uv.x*5.2+warp*2.3+t*.37,
                             flow*10.8-warp*1.4-t*2.20));
        float tissue=fbm(vec2(uv.x*6.7-fold*1.7,
                            flow*15.0+t*.97));
        // Turbulencia vertical del flujo, con nudos y pliegues a escalas
        // musicales (20-100 px), evitando las líneas constantes.
        float pulse=.5+.5*sin(flow*17.6-t*4.2+warp*7.8
                              +1.1*sin(uv.x*8.0+t*.8));
        float plume=fbm(vec2(uv.x*4.0+fold*.85+
                         .27*sin(flow*7.0-t*1.3),
                         flow*4.7-t*1.6));
        float plume2=fbm(vec2(uv.x*7.0+plume*2.2,
                              flow*11.7-t*3.5));
        float turbulent=smoothstep(.31,.68,
                .55*plume+.45*plume2+.24*(pulse-.5));
        float filament=pow(max(0.0,1.0-abs(sin(
                (uv.x+plume*.37+plume2*.2)*8.1+
                flow*5.6-t*1.4))),2.0)*pulse;
        // Corrientes blancas en forma de S: manchas diagonales gruesas,
        // no siete rayas verticales. Se arrastran hacia la tecla.
        float pathA=.5+.32*sin(flow*15.5-t*2.2+
                   .8*sin(flow*5.2-t*.73)+plume*1.1);
        float pathB=.5+.31*sin(flow*11.3+t*1.64+
                   1.3*sin(flow*4.3+t*.95)+plume2);
        float ridgeA=exp(-pow((uv.x-pathA)/.16,2.0));
        float ridgeB=exp(-pow((uv.x-pathB)/.12,2.0));
        float streak=max(ridgeA*(.30+.70*pulse),
                         ridgeB*(.23+.77*(1.0-pulse)));
        float energy=clamp(.10+.60*turbulent+.35*filament+
                           streak*.95,0.0,1.0);
        float inside=1.0-smoothstep(.84,1.04,edge);
        // Lenguas de llama que envuelven ambos lados de la nota.
        float tongue=.13+.29*plume+.09*sin(flow*10.0-t*1.3);
        float shell=exp(-pow((edge-1.0)/max(.06,tongue),2.0));
        float fringe=smoothstep(.33,.70,plume+.16*pulse)*shell;
        float emission=mask*uFuel*
               clamp(inside*energy*.98+fringe*.96,0.0,1.0);
        // A single moving forge front rides DOWN the note for 0.9 s
        // after its upper endpoint appears. It shapes luminous energy,
        // without hiding the MIDI geometry or creating repeating blasts.
        if(uBirth>.5 && uBirthAge>=0.0 && uBirthAge<1.2){
          float front=uBirthAge*150.0;
          float distance=uv.y*uExtent.y-front;
          float hotEdge=exp(-pow(distance/19.0,2.0));
          float forged=1.0-smoothstep(-12.0,21.0,distance);
          float ageFade=1.0-smoothstep(.55,1.20,uBirthAge);
          emission*=mix(1.0,clamp(.48+.52*forged+hotEdge*.65,0.0,1.35),
                         ageFade);
        }
        float hot=clamp(.43*turbulent+.46*filament+
                        streak*1.17,0.0,1.0);
        vec3 cold=uTint*.61+vec3(.02,.026,.045);
        vec3 hotTint=min(vec3(1.0),uTint*1.15+vec3(.06,.075,.105));
        vec3 light=mix(cold,hotTint,hot*.65);
        // High-contrast living fire inside the ORIGINAL 3D mesh.
        // Travel speed independent of the visible note's pixel height.
        // Repeated hot tongues do not persist at any one position.
        float phase=(pixelY+uTime*310.0)*.069;
        float shifting=fbm(vec2(uv.x*10.1+uTime*2.7,
                                pixelY*.063-uTime*7.9));
        float tip1=pow(max(0.0,sin(phase+shifting*2.9)),6.0);
        float tip2=pow(max(0.0,sin(phase*1.37+uv.x*2.7+2.1)),8.0);
        float localFlame=(.06+.79*tip1+.34*tip2)*
                         (1.0-smoothstep(.76,1.0,edge));
        float localAlpha=clamp(emission*localFlame*1.55,0.0,.72);
        vec3 coolFire=mix(uTint,vec3(.12,.66,.96),.61);
        vec3 warmFire=mix(uTint,vec3(1.0,.57,.16),.72);
        vec3 whiteHot=vec3(1.0,.96,.81);
        vec3 fire=mix(coolFire,warmFire,clamp(tip1*.85,0.0,1.0));
        fire=mix(fire,whiteHot,clamp(tip2*.56,0.0,.56));
        gl_FragColor=vec4(fire,localAlpha);
      }
    )GLSL";
    GLuint shaders[2]={};
    const GLenum types[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
    const char* srcs[2]={vert,frag};
    bool ok=true;
    for(int i=0;i<2;++i){
      shaders[i]=createShader(types[i]);
      shaderSource(shaders[i],1,&srcs[i],nullptr);
      compileShader(shaders[i]);
      GLint status=GL_FALSE;
      getShaderiv(shaders[i],GL_COMPILE_STATUS,&status);
      if(!status){
        char message[1600]={};
        GLsizei length=0;
        getShaderInfoLog(shaders[i],1599,&length,message);
        fprintf(stderr,"Linthesia GLSL flame compile error: %s\n",message);
        ok=false;
      }
    }
    if(ok){
      program=createProgram();
      for(auto shader:shaders)attachShader(program,shader);
      linkProgram(program);
      GLint linked=GL_FALSE;
      getProgramiv(program,GL_LINK_STATUS,&linked);
      if(!linked){
        char message[1600]={};
        GLsizei length=0;
        getProgramInfoLog(program,1599,&length,message);
        fprintf(stderr,"Linthesia GLSL flame link error: %s\n",message);
        deleteProgram(program);
        program=0;
      }else {
        // Las ubicaciones pertenecen al programa enlazado, no al fotograma.
        timeLocation=getUniformLocation(program,"uTime");
        contactLocation=getUniformLocation(program,"uContact");
        bloomLocation=getUniformLocation(program,"uBloom");
        birthLocation=getUniformLocation(program,"uBirth");
        birthAgeLocation=getUniformLocation(program,"uBirthAge");
        fuelLocation=getUniformLocation(program,"uFuel");
        widthLocation=getUniformLocation(program,"uWidth");
        originLocation=getUniformLocation(program,"uOrigin");
        extentLocation=getUniformLocation(program,"uExtent");
        tintLocation=getUniformLocation(program,"uTint");
        fprintf(stderr,"Linthesia GLSL flame: shader activo\n");
      }
    }
    for(auto shader:shaders)if(shader)deleteShader(shader);
  }
  bool draw(int x,int y,int w,int h,int xOffset,int yOffset,
            SDL_Color tint,float time,float remaining,float glow,
            bool contact=false,bool birth=false,float birthAge=-1.0f){
    init();
    if(!program)return false;
    const GLboolean depthWasEnabled=glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    useProgram(program);
    uniform1f(timeLocation,time);
    uniform1f(contactLocation,contact?1.0f:0.0f);
    uniform1f(birthLocation,birth&&!contact?1.0f:0.0f);
    uniform1f(birthAgeLocation,birthAge);
    // The impact used to be almost invisible at ordinary scene glow values:
    // its visual gain was multiplied twice by the small master glow.
    // Keep musical opacity, but give each short-lived ignition enough energy.
    const float visualGain=contact?
        std::min(2.1f,std::max(1.25f,glow/75.0f+1.05f)):
        std::min(1.28f,std::max(.35f,glow/82.0f));
    uniform1f(fuelLocation,remaining*visualGain);
    uniform1f(widthLocation,float(w));
    GLint vp[4]={};
    glGetIntegerv(GL_VIEWPORT,vp);
    uniform2f(originLocation,
              float(x+xOffset),float(vp[1]+vp[3]-(y+yOffset+h)));
    uniform2f(extentLocation,float(w),float(h));
    uniform3f(tintLocation,
              tint.r/255.0f,tint.g/255.0f,tint.b/255.0f);
    // The falling note remains narrower than its key. A very tight film
    // envelope avoids a broad fuzzy rectangle surrounding narrow columns.
    const float border=contact?
        std::max(5.0f,std::min(14.0f,w*.30f)):
        std::max(.65f,std::min(2.0f,w*.12f));
    for(int pass=0;pass<2;++pass){
      // Una única capa aditiva difusa y otra alfa para preservar el color.
      glBlendFunc(GL_SRC_ALPHA,pass==0?GL_ONE:GL_ONE_MINUS_SRC_ALPHA);
      uniform1f(bloomLocation,pass==0?1.0f:0.0f);
      glBegin(GL_QUADS);
      glTexCoord2f(-border/w,0);
      glVertex3f(x-border+xOffset,y+yOffset,.92f);
      glTexCoord2f(1+border/w,0);
      glVertex3f(x+w+border+xOffset,y+yOffset,.92f);
      glTexCoord2f(1+border/w,1);
      glVertex3f(x+w+border+xOffset,y+h+yOffset,.92f);
      glTexCoord2f(-border/w,1);
      glVertex3f(x-border+xOffset,y+h+yOffset,.92f);
      glEnd();
    }
    useProgram(0);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    if(depthWasEnabled)glEnable(GL_DEPTH_TEST);
    return true;
  }
};
NoteFlameFilm& flameFilm(){
  static NoteFlameFilm film;
  return film;
}
} // namespace

void Renderer::DrawNeonCapsule(int x,int y,int w,int h,int radius,
                               SDL_Color base,int opacity,int glow,bool shadow,
                               float phase,bool cinematic,float hitAge,
                               float birthAge,unsigned birthSeed,int birthTipY,float releaseFade){
  if(w<2 || h<2 || releaseFade<=.0f)return;
  releaseFade=std::max(0.0f,std::min(1.0f,releaseFade));
  ForceTexture(0);
  const bool birthing=cinematic && m_effect_birth==1 && birthAge>=0.0f;
  const int birthTop=birthTipY==-100000?y:std::max(y,birthTipY);
  const float birthClock=birthing?birthAge*m_effect_speed/100.0f:-1.0f;
  // The note is not present during the opening blast. Its visible body
  // materialises top-down AFTER the initial falling particle shower.
  const float reveal=birthing?
      TopBirthEmitter::Reveal(birthClock):1.0f;
  if(shadow && birthing && reveal<.999f)return;
  if(shadow){
    const SDL_Color shade1=ToColor(base.r/6,base.g/6,base.b/6,int(34*releaseFade));
    const SDL_Color shade2=ToColor(base.r/12,base.g/12,base.b/12,int(18*releaseFade));
    DrawRoundedGradient(x+1,y+2,w,h,
                        std::max(1,std::min(radius,w/2)),shade1,shade2);
    return;
  }
  if(birthing && reveal<=.001f){
    // First the ignition, then the particle shower; no note geometry yet.
    if(birthSeed!=0u)DrawTopBirthBurst(x,birthTop,w,base,birthAge,birthSeed,glow);
    return;
  }
  GLint oldScissor[4]={};
  const GLboolean hadScissor=glIsEnabled(GL_SCISSOR_TEST);
  bool clipRevealing=birthing && reveal<.999f;
  if(clipRevealing){
    glGetIntegerv(GL_SCISSOR_BOX,oldScissor);
    GLint vp[4]={};glGetIntegerv(GL_VIEWPORT,vp);
    int drawableW=0,drawableH=0;
    SDL_GL_GetDrawableSize(m_sdl_window,&drawableW,&drawableH);
    const float lh=std::max(1,UiAppearance::LogicalHeight(drawableW,drawableH));
    const float sy=vp[3]/lh;
    // Note's forging front travels from its upper cap to the keyboard.
    const float visibleHeight=std::max(1,y+h-birthTop);
    const int screenBottom=static_cast<int>(std::ceil(
        (birthTop+m_yoffset+visibleHeight*reveal)*sy));
    const int screenTop=static_cast<int>(std::floor((birthTop+m_yoffset)*sy));
    const int bottom=std::max(0,vp[1]+vp[3]-screenBottom);
    const int upper=std::min(vp[1]+vp[3],vp[1]+vp[3]-screenTop);
    const int left=hadScissor?oldScissor[0]:vp[0];
    const int right=hadScissor?oldScissor[0]+oldScissor[2]:vp[0]+vp[2];
    const int low=hadScissor?std::max(bottom,oldScissor[1]):bottom;
    const int high=hadScissor?std::min(upper,oldScissor[1]+oldScissor[3]):upper;
    glEnable(GL_SCISSOR_TEST);
    glScissor(left,low,std::max(0,right-left),std::max(0,high-low));
  }
  // Cada triángulo procede de Blender; los extremos se mantienen de
  // tamaño constante para que las notas MIDI largas no deformen sus tapas.
  // Respect the musician's configured transparency (the old cinematic
  // path silently forced 27%), while keeping a clear translucent prism.
  const float alpha=std::max(.13f,std::min(.78f,opacity/100.0f))*releaseFade;
  const float cap=std::max(2.0f,std::min({float(radius),
                                   w*.32f,h*.44f}));
  const float outer=std::max(0.0f,float(h)-2*cap);
  const float intensity=std::max(.50f,std::min(1.4f,
                                   1.09f+glow*.002f));
  glBegin(GL_TRIANGLES);
  for(size_t i=0;i<BlenderMesh::NoteCount;++i){
    const auto& v=BlenderMesh::Note[i];
    const float u=(v.x+.38f)/.76f;
    float screenY;
    if(v.y<.07f)screenY=y+(v.y/.07f)*cap;
    else if(v.y>.90f)screenY=y+h-(.97f-v.y)/.07f*cap;
    else screenY=y+cap+(v.y-.07f)/.83f*outer;
    const float screenX=x+w*u;
    const float diff=std::max(0.0f,
       -.33f*v.nx-.52f*v.ny+.79f*v.nz);
    const float spec=std::pow(std::max(0.0f,
       -.19f*v.nx-.10f*v.ny+.97f*v.nz),12.0f);
    const float light=(.65f+.34f*diff+.19f*spec)*intensity;
    SetColor(std::min(255.0f,base.r*light),
             std::min(255.0f,base.g*light),
             std::min(255.0f,base.b*light),
             static_cast<int>(255*alpha));
    const float raised=v.z*.24f;
    glVertex3f(screenX+m_xoffset,
               screenY-v.z*3.0f+m_yoffset,raised);
  }
  glEnd();
  // Translucent emissive glass edge on the ORIGINAL Blender note mesh.
  // Clear prism + bright rim; geometry, pitch, scale and MIDI duration stay.
  if(cinematic && glow>0 && !shadow){
    const GLboolean hadDepth=glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    const float power=std::min(1.0f,glow/100.f)*releaseFade;
    const float z=.74f;
    const SDL_Color rim=ToColor(std::min(255,int(base.r*.72f+90)),
                    std::min(255,int(base.g*.74f+84)),
                    std::min(255,int(base.b*.80f+78)));
    for(int side=0;side<2;++side){
      const float sx=(side==0?float(x):float(x+w));
      const float dir=(side==0?-1.f:1.f);
      const float haloW=std::max(2.0f,std::min(7.f,w*.20f));
      glBegin(GL_TRIANGLE_STRIP);
      for(int j=0;j<=18;++j){
        const float t=j/18.f;
        const float y0=y+h*t;
        const float shimmer=.79f+.21f*std::sin(t*16.8f-phase*8.6f+side*.8f);
        const float fade=std::min(1.f,std::min(t*28.f,(1.f-t)*28.f));
        SetColor(rim.r,rim.g,rim.b,int(156*power*fade*shimmer));
        glVertex3f(sx+m_xoffset,y0+m_yoffset,z);
        SetColor(rim.r,rim.g,rim.b,0);
        glVertex3f(sx+dir*haloW+m_xoffset,y0+m_yoffset,z);
      }
      glEnd();
    }
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    if(hadDepth)glEnable(GL_DEPTH_TEST);
  }

  // Llama fluida encapsulada: el combustible viaja DENTRO de la nota.
  // Este efecto no calcula órbitas alrededor del teclado: cada hebra sigue
  // la geometría de su propia columna MIDI hasta alcanzar el piano.
  const bool filmDrawn=cinematic && h>=10 && w>=5 &&
      flameFilm().draw(x,y,w,h,m_xoffset,m_yoffset,base,phase*(m_effect_speed/100.0f),
                       NoteEmitter::Smooth(10.0f,24.0f,float(h))*releaseFade,
                       float(glow),false,m_effect_birth==1,
                       birthAge>=0?birthAge*m_effect_speed/100.0f:-1.0f);
  if(cinematic && h>=10 && w>=5 && !filmDrawn){
    const GLboolean depthWasEnabled=glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    const auto clamp01=[](float v)->float{
      return std::max(0.0f,std::min(1.0f,v));
    };
    const float seconds=phase;
    // El combustible se libera al llegar al teclado; la llama interior
    // pierde energía progresivamente durante la nota sostenida.
    const float remaining=1.0f; // Intensidad constante hasta NoteOff
    const float power=std::min(1.35f,std::max(.25f,glow/80.0f))*
                      remaining*releaseFade;
    // Corrientes translúcidas y cáusticas no repetitivas, limitadas
    // estrictamente al ancho y alto de la nota.
    const int steps=std::min(112,std::max(18,h/4));
    for(int band=0;band<7;++band){
      const float lane=(band+.65f)/7.8f;
      const float rate=1.70f+.36f*(band%4);
      for(int veil=0;veil<2;++veil){
        glBegin(GL_TRIANGLE_STRIP);
        for(int j=0;j<=steps;++j){
          const float t=float(j)/steps;
          const float wave=std::sin(t*13.7f-seconds*rate+band*1.73f);
          const float detail=std::sin(t*29.4f+seconds*1.27f+band*.67f);
          const float middle=clamp01(lane+.067f*wave+.025f*detail);
          const float flare=.45f+.55f*
                 std::pow(.5f+.5f*std::sin(t*18.0f-seconds*(2.1f+band*.17f)
                                              +band*1.14f),2.0f);
          const float width=std::max(.45f,w*(veil==0?.095f:.039f))*
                            (.55f+flare*.9f);
          const float center=x+w*middle;
          const float attenuation=clamp01(std::min(t*20.0f,(1-t)*20.0f));
          const float lum=(.20f+.80f*flare)*attenuation*power*
                             (veil==0?.56f:1.0f);
          const int alpha=int((veil==0?65:163)*lum);
          const int cr=std::min(255,int(base.r*.45f+121));
          const int cg=std::min(255,int(base.g*.47f+117));
          const int cb=std::min(255,int(base.b*.48f+115));
          SetColor(cr,cg,cb,alpha);
          glVertex3f(std::max(float(x),center-width)+m_xoffset,
                     y+t*h+m_yoffset,.82f);
          glVertex3f(std::min(float(x+w),center+width)+m_xoffset,
                     y+t*h+m_yoffset,.82f);
        }
        glEnd();
      }
    }
    // Lenguas luminosas que envuelven LAS CARAS de la nota. Vuelan
    // verticalmente con ella, nunca giran alrededor del punto de impacto.
    for(int side=0;side<2;++side){
      const float dir=side==0?-1.0f:1.0f;
      const float edge=side==0?float(x):float(x+w);
      for(int i=0;i<12;++i){
        const float random=float(((i*37+side*71)*97)%101)/101.0f;
        const float path=std::fmod(i*47.0f+seconds*(25.0f+random*24.0f),
                                   float(std::max(40,h+52)));
        const float root=y+path-24.0f;
        const float length=23.0f+49.0f*random;
        glBegin(GL_TRIANGLE_STRIP);
        for(int j=0;j<=9;++j){
          const float t=j/9.0f;
          const float yy=root-length*t;
          const float outward=dir*(2.4f+5.8f*std::sin(t*3.14159265f)+
                             2.2f*std::sin(t*5.2f+seconds*2.2f+i));
          const float cx=edge+outward;
          const float thickness=std::max(.08f,1.2f+
                                (1-t)*1.6f*std::sin(t*3.14159265f));
          const float fade=clamp01(std::min((yy-y)/9.0f,
                                (y+h-yy)/9.0f))*(1-t);
          SetColor(std::min(255,int(base.r*.63f+78)),
                   std::min(255,int(base.g*.63f+78)),
                   std::min(255,int(base.b*.63f+78)),
                   int(124*fade*power));
          glVertex3f(cx-thickness+m_xoffset,yy+m_yoffset,.83f);
          glVertex3f(cx+thickness+m_xoffset,yy+m_yoffset,.83f);
        }
        glEnd();
      }
    }
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    if(depthWasEnabled)glEnable(GL_DEPTH_TEST);
  }

  // Núcleos de fluido luminoso. Se desplazan longitudinalmente
  // dentro de la cápsula, concentrándose y diluyéndose como llamas
  // contenidas: NO orbitan alrededor del teclado.
  if(cinematic && h>=18 && w>=5 && glow>0 && !filmDrawn){
    const float fuel=1.0f; // La llama no decae durante una nota sostenida
    const float amp=std::min(1.45f,glow/73.0f)*fuel*releaseFade;
    const GLboolean depthWasEnabled=glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    const int blobs=8; // Identidades estables; no se recolocan al cambiar h.
    for(int i=0;i<blobs;++i){
      // Y marcha hacia el teclado; el pequeño desplazamiento X se
      // mantiene DENTRO del ancho de la nota para que no sea un torbellino.
      const float offset=(i+.25f*std::sin(i*2.4f))/
                                     float(blobs);
      const float t=std::fmod(offset+
                       phase*(.125f+.008f*(i%4)),1.12f);
      if(t>1.0f)continue;
      const float centerY=y+t*h;
      const float centerX=x+w*(.5f+.23f*
                          std::sin(phase*1.65f+i*2.73f));
      const float flicker=.66f+.34f*
                            std::sin(phase*4.7f+i*3.13f);
      const float minFade=std::max(0.0f,std::min(1.0f,
              std::min(t*16.0f,(1.0f-t)*16.0f)));
      const float halfH=std::min(float(h)*.19f,
                             21.0f+28.0f*
                                  (.5f+.5f*std::sin(i*1.42f)));
      const float halfW=std::max(3.0f,w*(.23f+
                      .16f*(.5f+.5f*std::sin(i*3.74f))));
      for(int layer=0;layer<2;++layer){
        const float rr=layer==0?1.30f:.56f;
        glBegin(GL_TRIANGLE_FAN);
        const int cr=std::min(255,int(base.r*.58f+48.0f));
        const int cg=std::min(255,int(base.g*.62f+45.0f));
        const int cb=std::min(255,int(base.b*.65f+43.0f));
        SetColor(cr,cg,cb,std::min(150,int((layer==0?47:81)*
                                            amp*minFade*flicker)));
        glVertex3f(centerX+m_xoffset,centerY+m_yoffset,.85f);
        for(int j=0;j<=24;++j){
          const float angle=6.28318530718f*j/24.0f;
          // Gotas alargadas y asimétricas con borde turbulento.
          const float wave=.84f+.16f*
                  std::sin(angle*3.0f+phase*2.2f+i*.81f);
          const float xx=centerX+
                     std::cos(angle)*halfW*rr*wave;
          const float yy=centerY+
                     std::sin(angle)*halfH*rr*
                       (std::sin(angle)<0.0f?1.30f:.74f);
          SetColor(base.r,base.g,base.b,0);
          glVertex3f(xx+m_xoffset,yy+m_yoffset,.85f);
        }
        glEnd();
      }
    }
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    if(depthWasEnabled)glEnable(GL_DEPTH_TEST);
  }

  // Resplandor perimetral por adición: luz fuera de la cápsula,
  // NO otro rectángulo opaco que borre el escenario y el piano.
  if(glow>0){
    const float alphaScale=std::min(.9f,glow/115.0f)*releaseFade;
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    // Halo lateral de anchura oscilante; la envoltura es dinámica.
    // No crear un rectángulo de color constante a ambos lados.
    for(int pass=0;pass<(cinematic?2:1);++pass){
      // The halo must not visually exceed the key underneath the note.
      // Use a fine 1-2 px border, rather than the old 14+ px glow wings.
      const float border0=cinematic?
          (pass==0?std::max(.85f,w*.14f):
                   std::max(.55f,w*.08f)):
          std::max(.75f,std::min(2.3f,w*.18f));
      const float energy=cinematic?(pass==0?19.0f:31.0f):36.0f;
      const int slices=cinematic?std::min(80,std::max(16,h/9)):1;
      for(int side=0;side<2;++side){
        const float edge=side==0?float(x):float(x+w);
        const float sideSign=side==0?-1.0f:1.0f;
        glBegin(GL_TRIANGLE_STRIP);
        for(int j=0;j<=slices;++j){
          const float t=float(j)/slices;
          const float wave=.50f+.50f*std::sin(t*16.7f-
                       phase*2.0f+side*.81f+pass*1.7f);
          const float wrinkle=std::sin(t*29.5f+
                                      phase*1.3f+side*2.4f);
          const float widthMultiplier=.54f+.66f*wave+
                                      .09f*wrinkle;
          const float inner=edge+sideSign*std::min(.5f,
                            w*.035f)*std::sin(t*14.0f+
                                             phase*1.56f);
          const float outer=inner+sideSign*border0*widthMultiplier;
          const float fade=cinematic?
            std::min(1.0f,std::min(t*16.0f,(1.0f-t)*16.0f)):
            1.0f;
          SetColor(base.r,base.g,base.b,
            static_cast<int>(energy*alphaScale*
                          (.38f+.62f*wave)*fade));
          glVertex3f(inner+m_xoffset,y+t*h+m_yoffset,0);
          SetColor(base.r,base.g,base.b,0);
          glVertex3f(outer+m_xoffset,y+t*h+m_yoffset,0);
        }
        glEnd();
      }
    }
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
  }
  if(clipRevealing){
    glScissor(oldScissor[0],oldScissor[1],oldScissor[2],oldScissor[3]);
    if(!hadScissor)glDisable(GL_SCISSOR_TEST);
  }
  if(birthing && birthClock<TopBirthEmitter::Duration && birthSeed!=0u
     && releaseFade>=1.0f)
    DrawTopBirthBurst(x,birthTop,w,base,birthAge,birthSeed,glow);

}

void Renderer::DrawTopBirthBurst(int x,int y,int w,SDL_Color base,
                                 float age,unsigned seed,int glow){
  if(age<0 || age*m_effect_speed/100.0f>=TopBirthEmitter::Duration ||
     w<3 || m_effect_count<=0)
    return;
  ForceTexture(0);
  const unsigned count=std::min(72u,std::max(1u,unsigned(m_effect_count)/2));
  const float dispersion=m_effect_spread/100.0f;
  const float sizeMul=m_effect_size/100.0f;
  const float time=age*m_effect_speed/100.0f;
  const float cx=x+w*.5f;
  const GLboolean hadDepth=glIsEnabled(GL_DEPTH_TEST);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  const float pulse=1.0f-NoteEmitter::Smooth(.13f,.38f,time);
  // A broad but brief ignition halo, always limited to the note's track.
  if(pulse>.001f){
    const float cy=y+7.0f;
    // Double corona: a luminous ignition core and a much softer outer
    // burst, wide enough to be visible but below adjacent note spacing.
    const float baseRadius=std::max(13.0f,float(w)*(.68f+.38f*time));
    for(int halo=1;halo>=0;--halo){
      const float rx=baseRadius*(halo?1.45f:.86f);
      const float ry=rx*(halo?.64f:.43f);
      const int light=std::min(220,int((halo?55.0f:179.0f)*pulse));
      glBegin(GL_TRIANGLE_FAN);
      SetColor(base.r,base.g,base.b,light);
      glVertex3f(cx+m_xoffset,cy+m_yoffset,.94f);
      SetColor(base.r,base.g,base.b,0);
      for(int k=0;k<=32;++k){
        const float a=6.2831853f*k/32.0f;
        glVertex3f(cx+rx*std::cos(a)+m_xoffset,
                   cy+ry*std::sin(a)+m_yoffset,.94f);
      }
      glEnd();
    }
  }
  const float power=std::min(1.35f,std::max(.45f,glow/80.0f));
  glBegin(GL_TRIANGLES);
  for(unsigned i=0;i<count;++i){
    const auto p=TopBirthEmitter::Sample(time,i,seed,float(w),
                                        dispersion,sizeMul,count);
    if(p.alpha<.01f)continue;
    const float xx=cx+p.x+m_xoffset,yy=y+p.y+m_yoffset;
    const float r=p.size*1.25f;
    SetColor(base.r,base.g,base.b,std::min(215,int(204*p.alpha*power)));
    glVertex3f(xx,yy-r,.93f);
    glVertex3f(xx-r*.65f,yy+r,.93f);
    glVertex3f(xx+r*.65f,yy+r,.93f);
  }
  glEnd();
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_TRUE);
  if(hadDepth)glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawPianoContactLightBar(int x,int y,int width,int glow){
  if(width<=0 || glow<=0)return;
  ForceTexture(0);
  const float power=std::min(1.0f,glow/85.0f);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  // Arco cromático suave, de blanco azulado a turquesa y violeta rosado.
  // El tinte del teclado no cambia el color MIDI de cada nota.
  const SDL_Color colors[]={
      ToColor(126,201,255),ToColor(117,238,254),ToColor(85,247,209),
      ToColor(153,225,255),ToColor(196,157,254),ToColor(255,173,225)};
  const int segments=5;
  glBegin(GL_QUADS);
  for(int pass=0;pass<3;++pass){
    const float height=pass==0?30.0f:(pass==1?13.0f:2.0f);
    const int bottomAlpha=static_cast<int>((pass==0?57:
                                    (pass==1?133:208))*power);
    const int topAlpha=pass==2?bottomAlpha:
                       static_cast<int>((pass==0?0:7)*power);
    for(int j=0;j<segments;++j){
      const float x0=x+width*(static_cast<float>(j)/segments);
      const float x1=x+width*(static_cast<float>(j+1)/segments);
      const auto a=colors[j],b=colors[j+1];
      SetColor(a.r,a.g,a.b,topAlpha);
      glVertex3f(x0+m_xoffset,y-height+m_yoffset,0);
      SetColor(b.r,b.g,b.b,topAlpha);
      glVertex3f(x1+m_xoffset,y-height+m_yoffset,0);
      SetColor(b.r,b.g,b.b,bottomAlpha);
      glVertex3f(x1+m_xoffset,y+m_yoffset,0);
      SetColor(a.r,a.g,a.b,bottomAlpha);
      glVertex3f(x0+m_xoffset,y+m_yoffset,0);
    }
  }
  glEnd();
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_TRUE);
}

void Renderer::DrawPianoImpactParticles(int cx,int keyTop,int keyWidth,
                                        float elapsed,SDL_Color accent,
                                        unsigned seed,int strength,
                                        float animationTime,float releaseFade){
  // One MIDI NoteOn triggers a cinematic explosion, tall rising flame and
  // colored translucent smoke. It never stays still for an entire sustain.
  if(strength<=0 || keyWidth<3 || elapsed<0.0f ||
     elapsed>=1.10f || releaseFade<=0.0f)return;
  releaseFade=std::max(0.0f,std::min(1.0f,releaseFade));
  // La fase visual es continua entre golpes; la edad solo gobierna
  // nacimientos de partículas, nunca sustituye un emisor por otro.
  const float phase=(animationTime>=0.0f?animationTime:elapsed)*
                    (m_effect_speed/100.0f);
  const float attack=NoteEmitter::Smooth(0.0f,.10f,elapsed);
  ForceTexture(0);
  // Malla de emisión GPU derivada de la duración MIDI y del estado
  // efectivo de la tecla; mantiene el color del carril sin volverse blanca.
  // Emisor ceñido al carril: las llamas de un acorde no se fusionan.
  int windowW=1280,windowH=720;
  if(m_sdl_window)SDL_GetWindowSize(m_sdl_window,&windowW,&windowH);
  // Near HALF the live 3D scene, regardless of MIDI tempo or key width.
  const int plumeH=std::max(90,std::min(int(windowH*.47f),keyTop-95));
  const int plumeW=std::max(40,int(keyWidth*3.0f));
  flameFilm().draw(cx-plumeW/2,keyTop-plumeH,plumeW,plumeH,
                   m_xoffset,m_yoffset,accent,phase,.90f*attack*releaseFade,
                   float(strength),true,false,elapsed);
  const float power=std::min(1.02f,std::max(.10f,strength/105.0f))*releaseFade;
  const GLboolean depthWasEnabled=glIsEnabled(GL_DEPTH_TEST);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  // La luz retenida se libera de golpe en la superficie del piano.
  const float blast=NoteEmitter::Smooth(0.0f,.025f,elapsed)*
      (1.0f-NoteEmitter::Smooth(.025f,.23f,elapsed));
  if(blast>0.0f){
    glBegin(GL_TRIANGLE_FAN);
    SetColor(std::min(255,int(accent.r*.48f+127)),
             std::min(255,int(accent.g*.48f+117)),
             std::min(255,int(accent.b*.48f+104)),
             int(205*power*blast));
    glVertex3f(cx+m_xoffset,keyTop-3+m_yoffset,0);
    for(int i=0;i<=32;++i){
      const float a=6.2831853f*i/32.0f;
      const float radiusX=std::max(13.0f,keyWidth*1.5f)+
                           elapsed*110.0f;
      const float radiusY=12.0f+elapsed*92.0f;
      SetColor(accent.r,accent.g,accent.b,0);
      glVertex3f(cx+std::cos(a)*radiusX+m_xoffset,
                 keyTop-3+std::sin(a)*radiusY+m_yoffset,0);
    }
    glEnd();
  }
  // The old persistent triangle strip resembled a stationary candle.
  // Retain original rising particle emitter, 3D note bodies and GPU film;
  // do not place any fixed luminous rectangle on the key.
  // All four user parameters are consumed in the actual renderer.
  // Time scaling changes motion only; NoteOn/NoteOff is untouched.
  // Saturation control: rapid chords retain distinct colors, not a white blob.
  const unsigned activeCount=std::min(unsigned(m_effect_count)/2u,42u);
  const float visualAge=elapsed*m_effect_speed/100.0f;
  const float spread=std::min(1.35f,m_effect_spread/100.0f);
  const float particleSize=std::min(1.10f,m_effect_size/100.0f);
  glBegin(GL_TRIANGLES);
  for(unsigned i=0;i<activeCount;++i){
    const auto particle=NoteEmitter::Sample(visualAge,i,seed,float(keyWidth),
                                            spread,particleSize,activeCount);
    const float px=cx+particle.x;
    const float py=keyTop+particle.y;
    const float size=particle.size;
    const float opacity=1.0f-NoteEmitter::Smooth(.12f,.32f,elapsed);
    const int alpha=std::min(155,int(142.0f*particle.alpha*power*opacity));
    if(alpha<=0)continue;
    const float xx=px+m_xoffset,yy=py+m_yoffset;
    const int cr=std::min(255,int(32.0f+accent.r*.79f));
    const int cg=std::min(255,int(32.0f+accent.g*.79f));
    const int cb=std::min(255,int(32.0f+accent.b*.79f));
    // Estela real de 65 ms siguiendo la curva anterior de ESTA partícula.
    // No une generaciones distintas ni añade órbitas o segmentos fijos.
    // Mitad de las partículas para que los acordes conserven su fluidez.
    if((i&1u)==0u && alpha>14 && visualAge>.065f){
      const auto previous=NoteEmitter::Sample(
          visualAge-.065f,i,seed,float(keyWidth),
          spread,particleSize,activeCount);
      const float dx=particle.x-previous.x;
      const float dy=particle.y-previous.y;
      const float length2=dx*dx+dy*dy;
      if(previous.alpha>.055f && length2>1.0f && length2<420.0f){
        const float invLength=1.0f/std::sqrt(length2);
        const float nx=-dy*invLength,ny=dx*invLength;
        const float halfWidth=std::max(.32f,size*.46f);
        SetColor(cr,cg,cb,int(alpha*.51f));
        glVertex3f(xx+nx*halfWidth,yy+ny*halfWidth,.02f);
        glVertex3f(xx-nx*halfWidth,yy-ny*halfWidth,.02f);
        SetColor(accent.r,accent.g,accent.b,0);
        glVertex3f(cx+previous.x+m_xoffset,
                   keyTop+previous.y+m_yoffset,.02f);
      }
    }
    for(int j=0;j<4;++j){
      const float a=j*1.57079632679f;
      const float b=a+1.57079632679f;
      SetColor(cr,cg,cb,alpha);
      glVertex3f(xx,yy,0);
      SetColor(accent.r,accent.g,accent.b,0);
      glVertex3f(xx+std::cos(a)*size*2.8f,
                 yy+std::sin(a)*size*2.8f,0);
      glVertex3f(xx+std::cos(b)*size*2.8f,
                 yy+std::sin(b)*size*2.8f,0);
    }
  }
  glEnd();
  // Extra brief ignition synchronized with each NoteOn; these sparks
  // rise and become transparent in ~0.3 s, without replacing any 3D mesh.
  if(elapsed<ImpactFlameEmitter::Duration){
    glBegin(GL_TRIANGLES);
    for(unsigned i=0;i<24;++i){
      const auto spark=ImpactFlameEmitter::Sample(elapsed,i,seed,float(keyWidth),
                                                 spread,particleSize);
      if(spark.alpha<=0.0f)continue;
      const float sx=cx+spark.x+m_xoffset;
      const float sy=keyTop+spark.y+m_yoffset;
      const float radius=std::max(.35f,spark.radius);
      const int alpha=std::min(125,int(spark.alpha*112.0f*power));
      SDL_Color c=accent;
      if(spark.core>.82f){
        c.r=std::min(255,int(c.r*.46f+125));
        c.g=std::min(255,int(c.g*.46f+115));
        c.b=std::min(255,int(c.b*.46f+90));
      }
      SetColor(c.r,c.g,c.b,alpha);
      glVertex3f(sx,sy,.02f);
      SetColor(c.r,c.g,c.b,0);
      glVertex3f(sx-radius,sy+radius*2.7f,.02f);
      glVertex3f(sx+radius,sy+radius*2.7f,.02f);
    }
    glEnd();
  }
  // Swirling smoke wraps the traveling fire, never a frozen image.
  // Blended translucently (not additive) so pale clouds preserve the scene.
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  for(unsigned i=0;i<25;++i){
    const float delay=.016f*i+NoteEmitter::Random(seed,i*27u)*.09f;
    const float lifetime=.53f+.37f*NoteEmitter::Random(seed,i*61u+9u);
    const float life=(elapsed-delay)/lifetime;
    if(life<=0.0f||life>=1.0f)continue;
    const float rise=life*life*(3.0f-2.0f*life);
    const float height=(80.f+plumeH*.65f*NoteEmitter::Random(seed,i*23u+5u))*rise;
    const float drift=std::sin(life*8.0f+i*1.71f)*
                    (7.f+15.f*rise)*std::min(1.5f,spread);
    const float scatter=(NoteEmitter::Random(seed,i*97u+13u)-.5f)*
                         (13.f+26.f*rise)*std::min(1.5f,spread);
    const float sx=cx+drift+scatter+m_xoffset;
    const float sy=keyTop-height+m_yoffset;
    const float rad=(5.f+9.f*NoteEmitter::Random(seed,i*19u+31u))*(.8f+1.6f*rise);
    const float fade=NoteEmitter::Smooth(0.f,.14f,life)*
                   (1.f-NoteEmitter::Smooth(.48f,1.f,life));
    const int alpha=int(std::max(0.f,std::min(65.f,44.f*fade*power)));
    if(alpha<=0)continue;
    const int cr=std::min(255,int(.29f*accent.r+94));
    const int cg=std::min(255,int(.39f*accent.g+100));
    const int cb=std::min(255,int(.53f*accent.b+125));
    glBegin(GL_TRIANGLE_FAN);
    SetColor(cr,cg,cb,alpha);
    glVertex3f(sx,sy,.03f);
    for(int segment=0;segment<=16;++segment){
      const float angle=6.2831853f*segment/16.f;
      const float fractal=1.f+.15f*std::sin(angle*3.f+i*2.f+life*11.f);
      SetColor(cr,cg,cb,0);
      glVertex3f(sx+std::cos(angle)*rad*fractal,
                 sy+std::sin(angle)*rad*.85f*fractal,.03f);
    }
    glEnd();
  }
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_TRUE);
  if(depthWasEnabled)glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawStageLights(int x,int y,int w,int h,
                               int intensity,int tint,double elapsed,int movement){
  if(w<=0||h<=0||intensity<=0)return;
  const auto c=StageScenes::LightColor(tint);
  ForceTexture(0);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  glBegin(GL_TRIANGLES);
  for(int i=0;i<4;++i){
    const float phase=static_cast<float>(elapsed*.31+1.57*i);
    const float shift=movement/100.0f*.19f*std::sin(phase);
    const float origin=x+w*(.15f+.23f*i);
    const float end=x+w*(.17f+.22f*i+shift);
    const float nearTop=y+h*.07f;
    const float nearFloor=y+h*.98f;
    const int alpha=std::max(0,std::min(100,int(intensity*.50)));
    SetColor(c.r,c.g,c.b,alpha);
    glVertex3f(origin+m_xoffset,nearTop+m_yoffset,-.01f);
    SetColor(c.r,c.g,c.b,0);
    glVertex3f(end-h*.15f+m_xoffset,nearFloor+m_yoffset,-.01f);
    glVertex3f(end+h*.15f+m_xoffset,nearFloor+m_yoffset,-.01f);
  }
  glEnd();
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}
void Renderer::DrawQuad(int x, int y, int w, int h) {
  SelectTexture(0);

  // Los botones construidos con quads se hunden oscureciendo SU PROPIO
  // fondo durante la pulsación, sin añadir bordes rectangulares externos.
  // Evitar fondos de paneles, tarjetas grandes y zonas de selección.
  const int absoluteX=x+m_xoffset, absoluteY=y+m_yoffset;
  const bool pressed=pressState.held && w>=34 && w<=245 &&
                     h>=25 && h<=52 &&
                     pressState.x>=absoluteX && pressState.x<absoluteX+w &&
                     pressState.y>=absoluteY && pressState.y<absoluteY+h;
  GLfloat originalColor[4];
  if(pressed){
    glGetFloatv(GL_CURRENT_COLOR,originalColor);
    glColor4f(originalColor[0]*0.77f,
              originalColor[1]*0.77f,
              originalColor[2]*0.77f,
              originalColor[3]);
  }

  glBegin(GL_QUADS);
  glVertex3i(absoluteX,   absoluteY, 0);
  glVertex3i(absoluteX+w, absoluteY, 0);
  glVertex3i(absoluteX+w, absoluteY+h, 0);
  glVertex3i(absoluteX,   absoluteY+h, 0);
  glEnd();
  if(pressed)glColor4fv(originalColor);
}

void Renderer::DrawTga(const Tga *tga, int x, int y) const {
  DrawTga(tga, x, y, (int)tga->GetWidth(), (int)tga->GetHeight(), 0, 0);
}

void Renderer::DrawTga(const Tga *tga, int in_x, int in_y, int width, int height,
                       int src_x, int src_y) const {

  const int x = in_x + m_xoffset;
  const int y = in_y + m_yoffset;

  const double tx = static_cast<double>(src_x) / static_cast<double>(tga->GetWidth());
  const double ty = -static_cast<double>(src_y) / static_cast<double>(tga->GetHeight());
  const double tw = static_cast<double>(width) / static_cast<double>(tga->GetWidth());
  const double th = -static_cast<double>(height)/ static_cast<double>(tga->GetHeight());

  SelectTexture(tga->GetId());
  glBegin(GL_QUADS);
  glTexCoord2d(   tx,    ty); glVertex3i(      x,        y, 0);
  glTexCoord2d(   tx, ty+th); glVertex3i(      x, y+height, 0);
  glTexCoord2d(tx+tw, ty+th); glVertex3i(x+width, y+height, 0);
  glTexCoord2d(tx+tw,    ty); glVertex3i(x+width,        y, 0);
  glEnd();
}

void Renderer::DrawStretchedTga(const Tga *tga, int x, int y, int w, int h) const {
  DrawStretchedTga(tga, x, y, w, h, 0, 0, (int)tga->GetWidth(), (int)tga->GetHeight());
}

void Renderer::DrawStretchedTga(const Tga *tga, int x, int y, int w, int h,
                                int src_x, int src_y, int src_w, int src_h) const {
  const int sx = x + m_xoffset;
  const int sy = y + m_yoffset;

  const double tx =  static_cast<double>(src_x) / static_cast<double>(tga->GetWidth());
  const double ty = -static_cast<double>(src_y) / static_cast<double>(tga->GetHeight());
  const double tw =  static_cast<double>(src_w) / static_cast<double>(tga->GetWidth());
  const double th = -static_cast<double>(src_h) / static_cast<double>(tga->GetHeight());

  SelectTexture(tga->GetId());

  glBegin(GL_QUADS);
  glTexCoord2d(   tx,    ty); glVertex3i(  sx,   sy, 0);
  glTexCoord2d(   tx, ty+th); glVertex3i(  sx, sy+h, 0);
  glTexCoord2d(tx+tw, ty+th); glVertex3i(sx+w, sy+h, 0);
  glTexCoord2d(tx+tw,    ty); glVertex3i(sx+w,   sy, 0);
  glEnd();
}

void Renderer::DrawCenteredTga(const Tga *tga, int cx, int cy, int max_w, int max_h) const {

  double source_a = static_cast<double>(tga->GetWidth()) / tga->GetHeight();
  double max_a = static_cast<double>(max_w) / max_h;
  int w, h;
  if (max_a <= source_a)
  {
    w = max_w;
    h = max_w / source_a;
  } else
  {
    h = max_h;
    w = max_h * source_a;

  }
  DrawStretchedTga(tga, cx - w / 2, cy - h / 2, w, h);
}