// -*- mode: c++; coding: utf-8 -*-

// Linthesia

// Copyright (c) 2007 Nicholas Piegdon
// Adaptation to GNU/Linux by Oscar Aceña
// See COPYING for license information

#include "StatsState.h"
#include "LearningProgress.h"
#include "TrackSelectionState.h"
#include "PlayingState.h"
#include "Renderer.h"
#include "Textures.h"

#include <iomanip>
#include <algorithm>

using namespace std;

void StatsState::Init() {
  m_history=PracticeHistory::Load(m_state.song_path);
  m_back_button = ButtonState(
      Layout::ScreenMarginX,
      GetStateHeight() - Layout::ScreenMarginY/2 - Layout::ButtonHeight/2,
      Layout::ButtonWidth, Layout::ButtonHeight);

  m_continue_button = ButtonState(
      GetStateWidth() - Layout::ScreenMarginX - Layout::ButtonWidth,
      GetStateHeight() - Layout::ScreenMarginY/2 - Layout::ButtonHeight/2,
      Layout::ButtonWidth, Layout::ButtonHeight);
}

void StatsState::Resize() {
    m_back_button.SetX(Layout::ScreenMarginX);
    m_back_button.SetY(GetStateHeight() - Layout::ScreenMarginY/2 - Layout::ButtonHeight/2);

    m_continue_button.SetX(GetStateWidth() - Layout::ScreenMarginX - Layout::ButtonWidth);
    m_continue_button.SetY(GetStateHeight() - Layout::ScreenMarginY/2 - Layout::ButtonHeight/2);
}

void StatsState::Update() {

  MouseInfo mouse = Mouse();

  m_continue_button.Update(mouse);
  m_back_button.Update(mouse);

  if (IsKeyPressed(KeyEscape) || m_back_button.hit) {
    ChangeState(new TrackSelectionState(m_state));
    return;
  }

  if (IsKeyPressed(KeyEnter) || m_continue_button.hit) {
    ChangeState(new PlayingState(m_state));
    return;
  }

  m_tooltip = "";
  if (m_back_button.hovering)
    m_tooltip = "Volver a la selección de pistas.";

  if (m_continue_button.hovering)
    m_tooltip = "Volver a tocar esta canción con los mismos ajustes.";
}

void StatsState::Draw(Renderer &renderer) const {

  const bool ConstrainedHeight = (GetStateHeight() < 720);

  int left = GetStateWidth() / 2 + 40;
  const int InstructionsY = ConstrainedHeight ? 120 : 263;

  renderer.SetColor(White);
  renderer.DrawTga(GetTexture(StatsText), left - 270, InstructionsY - 113);

  if (!ConstrainedHeight) {
    Layout::DrawTitle(renderer, m_state.song_title);
    Layout::DrawHorizontalRule(renderer, GetStateWidth(), Layout::ScreenMarginY);
  }

  Layout::DrawHorizontalRule(renderer, GetStateWidth(), GetStateHeight() - Layout::ScreenMarginY);

  Layout::DrawButton(renderer, m_continue_button, GetTexture(ButtonRetrySong));
  Layout::DrawButton(renderer, m_back_button, GetTexture(ButtonChooseTracks));

  const SongStatistics &s = m_state.stats;

  double hit_percent = 0.0;
  if (s.notes_user_could_have_played > 0) {
    hit_percent = 100.0 * (s.notes_user_actually_played / (s.notes_user_could_have_played * 1.0));
  }

  string grade = "F";
  if (hit_percent >= 50) grade = "D-";
  if (hit_percent >= 55) grade = "D";
  if (hit_percent >= 63) grade = "D+";
  if (hit_percent >= 70) grade = "C-";
  if (hit_percent >= 73) grade = "C";
  if (hit_percent >= 77) grade = "C+";
  if (hit_percent >= 80) grade = "B-";
  if (hit_percent >= 83) grade = "B";
  if (hit_percent >= 87) grade = "B+";
  if (hit_percent >= 90) grade = "A-";
  if (hit_percent >= 93) grade = "A";
  if (hit_percent >= 97) grade = "A+";
  if (hit_percent >= 99) grade = "A++";
  if (hit_percent >= 100) grade = "A+++";

  int stray_percent = 0;
  if (s.total_notes_user_pressed > 0)
    stray_percent = static_cast<int>((100.0 * s.stray_notes) / s.total_notes_user_pressed);

  int average_speed = 0;
  if (s.notes_user_could_have_played > 0)
    average_speed = s.speed_integral / s.notes_user_could_have_played;

   // Choose a dynamic color for the grade
  const double p = hit_percent / 100.0;
  const double r = max(0.0, 1 - (p*p*p*p));
  const double g = max(0.0, 1 - (((p-  1)*4)*((p-  1)*4)));
  const double b = max(0.0, 1 - (((p-.75)*5)*((p-.75)*5)));

  const SDL_Color c = Renderer::ToColor(int(r*0xFF), int(g*0xFF), int(b*0xFF));

  TextWriter grade_text(left - 5, InstructionsY - 15, renderer, false, 100);
  grade_text << Text(grade, c);

  TextWriter score(left, InstructionsY + 112, renderer, false, 28);
  score << STRING(static_cast<int>(s.score));

  TextWriter speed(left, InstructionsY + 147, renderer, false, 28);
  speed << STRING(average_speed << " %");

  TextWriter good(left, InstructionsY + 218, renderer, false, 28);
  good << STRING(s.notes_user_actually_played << " / " <<
                 s.notes_user_could_have_played << "  (" <<
                 static_cast<int>(hit_percent) << " %" << ")");

  TextWriter stray(left, InstructionsY + 255, renderer, false, 28);
  stray << STRING(s.stray_notes << "  (" << stray_percent << " %" << ")");

  TextWriter combo(left, InstructionsY + 323, renderer, false, 28);
  combo << STRING(s.longest_combo);

  TextWriter tooltip(GetStateWidth() / 2,
                     GetStateHeight() - Layout::ScreenMarginY/2 - Layout::TitleFontSize/2,
                     renderer, true, Layout::TitleFontSize);

  tooltip << m_tooltip;

  // Session history, not merely the historical best raw score.
  // A visible trend helps the learner decide which hand to practise next.
  const int hx=28;
  const int hy=ConstrainedHeight?185:270;
  renderer.SetColor(18,38,51,230);
  renderer.DrawQuad(hx-8,hy-14,
              std::min(390,GetStateWidth()/2-44),243);
  renderer.SetColor(94,165,182);
  renderer.DrawQuad(hx-8,hy-14,
              std::min(390,GetStateWidth()/2-44),2);
  renderer.SetColor(255,255,255);
  TextWriter heading(hx,hy,renderer,false,18);
  heading<<Text("HISTORIAL DE PRÁCTICA",
                Renderer::ToColor(225,245,250));
  if(m_history.empty()){
    TextWriter empty(hx,hy+36,renderer,false,14);
    empty<<Text("Sin sesiones evaluadas todavía",
                Renderer::ToColor(180,208,222));
  }else{
    const auto learning=LearningProgress::Analyze(m_history);
    renderer.SetColor(255,255,255);
    TextWriter advancement(hx,hy+182,renderer,false,13);
    advancement<<Text(STRING("Progreso: "<<learning.points
                 <<"/100 · "<<learning.next),
                 Renderer::ToColor(214,234,156));
    int best=0;
    for(const auto& a:m_history)if(a.complete)
      best=std::max(best,a.Accuracy());
    const auto& latest=m_history.front();
    TextWriter summary(hx,hy+28,renderer,false,13);
    summary<<Text(STRING("Última: "<<latest.Accuracy()<<"%   Mejor: "
              <<best<<"%   Sesiones: "<<m_history.size()),
              Renderer::ToColor(180,224,210));
    for(size_t i=0;i<m_history.size()&&i<5;++i){
      const auto& a=m_history[i];
      const int y=hy+56+int(i)*28;
      renderer.SetColor(255,255,255);
      TextWriter line(hx,y,renderer,false,13);
      line<<Text(STRING("#"<<i+1<<"  "
            <<a.Accuracy()<<"%   "<<a.hit<<"/"<<a.expected
            <<" notas   "<<a.errors<<" errores   "<<a.bpm<<" BPM"
            <<(a.complete?"":" · parcial")),
            Renderer::ToColor(i==0?243:186,i==0?236:204,
                              i==0?188:218));
    }
    if(m_history.size()>=2){
      const int trend=latest.Accuracy()-m_history[1].Accuracy();
      renderer.SetColor(255,255,255);
      TextWriter difference(hx,hy+202,renderer,false,12);
      difference<<Text(STRING("Variación reciente: "
                <<(trend>0?"+":"")<<trend<<" puntos"),
                Renderer::ToColor(180,225,227));
    }
  }
}
