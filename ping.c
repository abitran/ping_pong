#include <SDL3/SDL_error.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL3/SDL_main.h>

#define SDL_FLAGS SDL_INIT_VIDEO
#define WIDTH 800
#define HEIGHT 600
#define BALL_SPEED 400
#define PADDLE_SPEED 500
#define PADDLE_HEIGHT 100
#define PADDLE_WIDTH 20
#define BALL_SIZE 10
#define LINE_STEP 30

// Paddle has a (x,y) position and a vertical velocity (vy)
typedef struct paddle_s {
  float x, y;
  float vy;
  int score;
  TTF_Text *score_text;
} paddle_s;

// Ball has a (x,y) position and a horizontal (vx) and vertical (vy) vy
typedef struct ball_s {
  float x, y;
  float vx, vy;
} ball_s;

// Game structure

typedef struct gamestate {
  SDL_Window *window;
  SDL_Renderer *renderer;
  ball_s ball;
  paddle_s rpaddle;
  paddle_s lpaddle;
  Uint64 last_ticks;
  TTF_TextEngine *engine;
  TTF_Font *font;
} gamestate;

bool check_collisions(SDL_FRect a, SDL_FRect b);
void game_init(gamestate *state);
void reset_ball(ball_s *ball);

// Initialization step
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

  srand((unsigned int)time(NULL));
  gamestate *state = SDL_calloc(1, sizeof(gamestate));
  if (!state)
    return SDL_APP_FAILURE;

  *appstate = state;

  SDL_SetAppMetadata("my app", "1.0", "com.example.renderer-clear");

  if (!SDL_Init(SDL_FLAGS)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", WIDTH, HEIGHT, 0,
                                   &state->window, &state->renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  SDL_SetRenderLogicalPresentation(state->renderer, WIDTH, HEIGHT,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);

  if (!TTF_Init()) {
    SDL_Log("TTF Init Failed: %s", SDL_GetError());
  }

  game_init(state);
  state->font =
      TTF_OpenFont("/usr/share/fonts/Adwaita/AdwaitaMono-Bold.ttf", 60);
  state->engine = TTF_CreateRendererTextEngine(state->renderer);
  if (!state->font) {
    SDL_Log("Can't load the font: %s", SDL_GetError());
  }

  state->rpaddle.score_text =
      TTF_CreateText(state->engine, state->font, "Score: 0", 0);
  state->lpaddle.score_text =
      TTF_CreateText(state->engine, state->font, "Score: 0", 0);

  TTF_SetTextColor(state->rpaddle.score_text, 255, 255, 255, 255);
  TTF_SetTextColor(state->lpaddle.score_text, 255, 255, 255, 255);
  reset_ball(&state->ball);
  state->last_ticks = SDL_GetTicks();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }

  if (event->type == SDL_EVENT_KEY_DOWN) {
    if (event->key.key == SDLK_ESCAPE)
      return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
}
// Iterate

SDL_AppResult SDL_AppIterate(void *appstate) {
  Uint64 now = SDL_GetTicks();
  gamestate *state = (gamestate *)appstate;
  float dt = (now - state->last_ticks) / 1000.0f;
  state->last_ticks = now;

  state->ball.x += state->ball.vx * dt;
  state->ball.y += state->ball.vy * dt;

  if (state->ball.y <= 0) {
    state->ball.y = 0;
    state->ball.vy = -state->ball.vy;
  }

  if (state->ball.y + BALL_SIZE >= HEIGHT) {
    state->ball.y = HEIGHT - BALL_SIZE;
    state->ball.vy = -state->ball.vy;
  }

  if (state->ball.x < 0) {
    state->rpaddle.score++;
    reset_ball(&state->ball);
  }

  if (state->ball.x > WIDTH + BALL_SIZE) {
    state->lpaddle.score++;
    reset_ball(&state->ball);
  }

  const bool *keystate = SDL_GetKeyboardState(NULL);

  if (keystate[SDL_SCANCODE_W])
    state->lpaddle.y -= state->lpaddle.vy * dt;
  if (keystate[SDL_SCANCODE_S])
    state->lpaddle.y += state->lpaddle.vy * dt;

  if (keystate[SDL_SCANCODE_UP])
    state->rpaddle.y -= state->rpaddle.vy * dt;
  if (keystate[SDL_SCANCODE_DOWN])
    state->rpaddle.y += state->rpaddle.vy * dt;

  // Boundaries

  if (state->lpaddle.y < 0)
    state->lpaddle.y = 0;

  if (state->lpaddle.y > HEIGHT - PADDLE_HEIGHT)
    state->lpaddle.y = HEIGHT - PADDLE_HEIGHT;

  if (state->rpaddle.y < 0)
    state->rpaddle.y = 0;

  if (state->rpaddle.y > HEIGHT - PADDLE_HEIGHT)
    state->rpaddle.y = HEIGHT - PADDLE_HEIGHT;

  SDL_FRect rect_rpaddle = {state->rpaddle.x, state->rpaddle.y, PADDLE_WIDTH,
                            PADDLE_HEIGHT};
  SDL_FRect rect_lpaddle = {state->lpaddle.x, state->lpaddle.y, PADDLE_WIDTH,
                            PADDLE_HEIGHT};
  SDL_FRect rect_ball = {state->ball.x, state->ball.y, BALL_SIZE, BALL_SIZE};

  if (check_collisions(rect_ball, rect_lpaddle)) {
    state->ball.vx = BALL_SPEED * 1.05f;
    state->ball.x = state->lpaddle.x + PADDLE_WIDTH;
    state->ball.vy += (rand() % 40 - 20);
  }

  if (check_collisions(rect_ball, rect_rpaddle)) {
    state->ball.vx = -BALL_SPEED * 1.05f;
    state->ball.x = state->rpaddle.x - BALL_SIZE;
    state->ball.vy += (rand() % 40 - 20);
  }

  char buf1[32], buf2[32];

  snprintf(buf1, sizeof(buf1), "%d", state->lpaddle.score);
  snprintf(buf2, sizeof(buf2), "%d", state->rpaddle.score);

  SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(state->renderer);

  SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(state->renderer, &rect_rpaddle);
  SDL_RenderFillRect(state->renderer, &rect_lpaddle);
  SDL_RenderFillRect(state->renderer, &rect_ball);

  TTF_SetTextString(state->lpaddle.score_text, buf1, 0);
  TTF_SetTextString(state->rpaddle.score_text, buf2, 0);
  
  TTF_DrawRendererText(state->lpaddle.score_text, 300.0f, 20.0f);
  TTF_DrawRendererText(state->rpaddle.score_text, 450.0f, 20.0f);

  SDL_SetRenderDrawColor(state->renderer, 65, 65, 64, SDL_ALPHA_OPAQUE);

  for (float y = 0; y < HEIGHT; y += LINE_STEP) {
    SDL_FRect net = {(WIDTH / 2.0f) - 2.0f, y, 4.0f, 5.0f};
    SDL_RenderFillRect(state->renderer, &net);
  }

  SDL_RenderPresent(state->renderer);
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  if (appstate) {
    gamestate *state = (gamestate *)appstate;
    TTF_DestroyText(state->lpaddle.score_text);
    TTF_DestroyText(state->rpaddle.score_text);
    TTF_DestroyRendererTextEngine(state->engine);
    TTF_CloseFont(state->font);
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    TTF_Quit();
    SDL_free(state);
  }
}

bool check_collisions(SDL_FRect a, SDL_FRect b) {
  return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h &&
          a.y + a.h > b.y);
}

void game_init(gamestate *state) {
  state->lpaddle.x = 30.0f;
  state->lpaddle.y = (HEIGHT / 2.0f) - (PADDLE_HEIGHT / 2.0f);
  state->lpaddle.vy = PADDLE_SPEED;
  state->lpaddle.score = 0;

  state->rpaddle.x = WIDTH - 30.0f - PADDLE_WIDTH;
  state->rpaddle.y = (HEIGHT / 2.0f) - (PADDLE_HEIGHT / 2.0f);
  state->rpaddle.vy = PADDLE_SPEED;
  state->rpaddle.score = 0;

  state->last_ticks = SDL_GetTicks();
}

void reset_ball(ball_s *ball) {
  ball->x = (WIDTH / 2.0f) - (BALL_SIZE / 2.0f);
  ball->y = (HEIGHT / 2.0f) - (BALL_SIZE / 2.0f);
  ball->vx = (rand() % 2 == 0) ? BALL_SPEED : -BALL_SPEED;
  ball->vy = (rand() % 2 == 0) ? BALL_SPEED : -BALL_SPEED;
}
