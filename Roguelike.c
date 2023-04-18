#pragma region init
// #pragma CC optimize("fsanitize=undefined, fsanitize=address, Wall")
// #define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include "SFMT.h"

#define SIZE_SEED 32
#define ROOM_NUM 3
int **map;
#pragma endregion init

typedef struct room {
  int Lx;
  int Ly;
  int Rx;
  int Ry;
  // 割当領域(左上と右下)
  int height;
  int width;
  // 実際の部屋の大きさ
  int left_way;   // Ly
  int right_way;  // Ry
  int above_way;  // Lx
  int below_way;  // Rx
  // 道の座標
  struct room *right_room;
  struct room *left_room;
  struct room *above_room;
  struct room *below_room;
  // 4方向連結リスト
} Room;

typedef struct {
  Room *Start;
  Room *End;
} Rooms;

typedef struct {
  int x;
  int y;
  // Player cordinate
  int HP;
  int basic_attack;
} player;

typedef struct enemy {
  int x;
  int y;
  // Enemy cordinate
  int HP;
  int basic_attack;
  struct enemy *next;
  struct enemy *prev;
} Enemy;

typedef struct {
  Enemy *Start;
  Enemy *End;
} Enemys;

Enemys *enemys;

player set_player(int x, int y, int HP, int basic_attack) {
  player res;
  res.x = x;
  res.y = y;
  res.HP = HP;
  res.basic_attack = basic_attack;
  return res;
}

Enemy *set_enemy(int x, int y, int HP, int basic_attack, Enemy *prev) {
  Enemy *res = malloc(sizeof(Enemy));
  res->x = x;
  res->y = y;
  res->HP = HP;
  res->basic_attack = basic_attack;
  res->prev = prev;
  res->next = NULL;
  return res;
}

Room *make_room(Room *left, Room *above) {
  Room *res = malloc(sizeof(Room));
  if (res != NULL) {
    res->left_room = left;
    res->above_room = above;
  }
  res->right_room = NULL;
  res->below_room = NULL;
  res->Lx = res->Ly = res->Rx = res->Ry = 0;
  res->height = res->width = 0;
  if (left != NULL) {
    left->right_room = res;
  }
  if (above != NULL) {
    above->below_room = res;
  }
  return res;
}

Rooms *room_init(void) {
  Rooms *res = malloc(sizeof(Rooms));
  if (res != NULL) {
    res->Start = make_room(NULL, NULL);
  }
  if (res->Start == NULL) {
    free(res);
    return NULL;
  }
  return res;
}

void delate_room(Room *r) {
  Room *left = r->left_room;
  Room *right = r->right_room;
  Room *above = r->above_room;
  Room *below = r->below_room;
  left->right_room = right;
  right->left_room = left;
  above->below_room = below;
  below->above_room = above;
  free(r);
  return;
  // R R R    R R R
  // R R R -> R   R
  // R R R    R R R 各方面のポインタの付け替え
}

int min(int x, int y) {
  if (x < y) return x;
  return y;
}

int max(int x, int y) {
  if (x > y) return x;
  return y;
}

int abs(int x) {
  if (x > 0) return x;
  return -x;
}

void print_map(int x, int y, int seed, int floor) {
  FILE *fp;
  int i, j;
  char *name = (char*) malloc(256 * sizeof(char));
  sprintf(name, "map_%d_%d", seed, floor);
  if ((fp = fopen(name, "w")) == NULL) {
    printf("Do not open the file 'map' \n");
  } else {
    for (i = 0; i <= y; i++) {
      for (j = 0; j <= x; j++) {
        if (map[i][j] == 0)
          fprintf(fp, "■ ");
        else if (map[i][j] == 2)
          fprintf(fp, "G  ");
        else
          fprintf(fp, "   ");
      }
      fprintf(fp, "\n");
    }
    fclose(fp);
  }
}

void map_init(int x, int y) {
  int i, j;
  map = (int **)malloc((y + 10) * sizeof(int *));
  if (map == NULL) return;
  for (i = 0; i < y + 5; i++) {
    map[i] = (int *)malloc((x + 10) * sizeof(int));
    if (map[i] == NULL) {
      map = NULL;
      return;
    }
    for (j = 0; j < x + 5; j++) {
      map[i][j] = 0;
    }
  }
};

Enemys *enemy_init(void) {
  int i;
  Enemys *res = malloc(sizeof(Enemys));
  if (res != NULL) {
    Enemy *now = res->Start = set_enemy(0, 0, 10, 1, NULL);
    for (i = 0; i < ROOM_NUM * ROOM_NUM * 2 - 1; i++) {
      now->next = set_enemy(0, 0, 10, 1, now);
      now = now->next;
    }
  }
  return res;
}

void view_map(int x, int y, player *p) {
  int i, j;
  for (i = 0; i <= y; i++) {
    for (j = 0; j <= x; j++) {
      if (abs(i - p->y) > 10 || abs(j - p->x) > 10) continue;
      if (i == p->y && j == p->x) {
        printf("P ");
        continue;
      }
      Enemy *enemy = enemys->Start;
      int flg = 0;
      while (enemy != NULL) {
        if (enemy->x == j && enemy->y == i) {
          flg = 1;
          break;
        }
        enemy = enemy->next;
      }
      if (flg)
        printf("E ");
      else if (map[i][j] == 0)
        printf("■ ");
      else if (map[i][j] == 2)
        printf("G ");
      else
        printf("  ");
    }
    if (abs(i - p->y) > 10) continue;
    printf("\n");
  }
  printf("\n HP: %d  \n\n", p->HP);
  printf("移動: WASD, 攻撃: K\n");
  printf("Action: ");
  // プレイヤーからの距離+-10くらいまでの表示にする
}

void create_map(int seed, Rooms *R) {
  int i, j, k, l, cnt = 0;
  if (R == NULL) return;
  Room *left = R->Start;
  Room *above = R->Start;
  for (i = 1; i < ROOM_NUM; i++) {
    above->right_room = make_room(above, NULL);
    if (above->right_room == NULL) {
      i--;
    } else {
      above = above->right_room;
    }
  }
  for (i = 2; i < ROOM_NUM; i++) {
    if (above->left_room != NULL) {
      above = above->left_room;
    }
  }
  for (i = 1; i < ROOM_NUM; i++) {
    left->below_room = make_room(NULL, left);
    if (left->below_room == NULL) {
      i--;
    } else {
      left = left->below_room;
    }
  }
  for (i = 2; i < ROOM_NUM; i++) {
    if (left->above_room != NULL) {
      left = left->above_room;
    }
  }
  for (i = 1; i < ROOM_NUM; i++) {
    for (j = 1; j < ROOM_NUM; j++) {
      left->right_room = make_room(left, above);
      if (j != ROOM_NUM - 1) {
        left = left->right_room;
        above = above->right_room;
      }
    }
    if (i == ROOM_NUM - 1) {
      R->End = left->right_room;
    }
    for (j = 2; j < ROOM_NUM; j++) {
      if (left->left_room != NULL) {
        left = left->left_room;
      }
      if (above->left_room != NULL) {
        above = above->left_room;
      }
    }
    if (left->below_room != NULL) {
      left = left->below_room;
    }
    if (above->below_room != NULL) {
      above = above->below_room;
    }
  }
  // ここまで連結リストの初期生成
  // ここから部屋の領域割当

  sfmt_t sfmt;
  int div, div_s = 0;
  sfmt_init_gen_rand(&sfmt, seed);
  int goal = sfmt_genrand_uint32(&sfmt) % (ROOM_NUM * ROOM_NUM);
  Room *now = R->Start;

  above = left = R->Start;
  for (i = 0; i < 5; i++) {
    do {
      div = sfmt_genrand_uint32(&sfmt) % SIZE_SEED;
    } while (div < 7);
    for (j = 0; j < 5; j++) {
      now->Rx = div + div_s;
      if (now->below_room != NULL) {
        now = now->below_room;
      }
    }
    while (now->above_room != NULL) {
      now = now->above_room;
    }

    if (now->right_room != NULL) {
      now = now->right_room;
    }
    div_s += div;
  }
  div_s = 0;
  now = R->Start;
  for (i = 0; i < 5; i++) {
    do {
      div = sfmt_genrand_uint32(&sfmt) % SIZE_SEED;
    } while (div < 7);
    for (j = 0; j < 7; j++) {
      now->Ry = div + div_s;
      if (now->right_room != NULL) {
        now = now->right_room;
      }
    }
    for (j = 0; j < 7; j++) {
      if (now->left_room != NULL) {
        now = now->left_room;
      }
    }
    if (now->below_room != NULL) {
      now = now->below_room;
    }
    div_s += div;
  }

  now = R->Start;
  while (1) {
    while (1) {
      if (now->left_room == NULL) {
        now->Lx = 1;
      } else {
        now->Lx = now->left_room->Rx;
      }
      if (now->above_room == NULL) {
        now->Ly = 1;
      } else {
        now->Ly = now->above_room->Ry;
      }
      if (now->right_room != NULL) {
        now = now->right_room;
      } else {
        break;
      }
    }
    while (1) {
      if (now->left_room != NULL) {
        now = now->left_room;
      } else {
        break;
      }
    }
    if (now->below_room != NULL) {
      now = now->below_room;
    } else {
      break;
    }
  }
  now = R->Start;
  // ここから実際の部屋作成
  map_init(R->End->Rx, R->End->Ry);
  now = R->Start;
  Enemy *enemy = enemys->Start;
  while (1) {
    while (1) {
      now->width = (sfmt_genrand_uint32(&sfmt) % (now->Rx - now->Lx) / 4) +
                   (now->Rx - now->Lx) / 2;
      now->height = (sfmt_genrand_uint32(&sfmt) % (now->Ry - now->Ly) / 4) +
                    (now->Ry - now->Ly) / 2;
      now->above_way = sfmt_genrand_uint32(&sfmt) % now->width;
      now->below_way = sfmt_genrand_uint32(&sfmt) % now->width;
      now->left_way = sfmt_genrand_uint32(&sfmt) % now->height;
      now->right_way = sfmt_genrand_uint32(&sfmt) % now->height;
      if (now->right_room != NULL) {
        now = now->right_room;
      } else {
        break;
      }
    }
    while (now->left_room != NULL) {
      now = now->left_room;
    }
    if (now->below_room != NULL) {
      now = now->below_room;
    } else {
      break;
    }
  }

  now = R->Start;
  while (1) {
    while (1) {
      int v = sfmt_genrand_uint32(&sfmt) % 5,
          u = sfmt_genrand_uint32(&sfmt) % 5;
      if (now->left_room == NULL && now->above_room == NULL) {
        v = 0;
        u = 0;
      } else if (now->above_room == NULL) {
        if (now->Lx + u - now->left_room->Rx < 3)
          u = now->Lx - now->left_room->Rx + 3;
        now->Lx += u;
        int tmp = sfmt_genrand_uint32(&sfmt) % (now->Lx - now->left_room->Lx -
                                                now->left_room->width - 2) +
                  1;
        for (i = now->left_room->Lx + now->left_room->width;
             i < (now->left_room->Lx + now->left_room->width + tmp); i++) {
          map[now->left_room->Ly + now->left_room->right_way][i] = 1;
        }
        for (i = (now->left_room->Lx + now->left_room->width + tmp);
             i < now->Lx; i++) {
          map[now->Ly + now->left_way][i] = 1;
        }
        for (i = min(now->left_room->Ly + now->left_room->right_way,
                     now->Ly + now->left_way);
             i <= max(now->left_room->Ly + now->left_room->right_way,
                      now->Ly + now->left_way);
             i++) {
          map[i][now->left_room->Lx + now->left_room->width + tmp] = 1;
        }
      } else if (now->left_room == NULL) {
        if (now->Ly + v - now->above_room->Ry < 3)
          v = now->Ly - now->above_room->Ry + 3;
        now->Ly += v;
        int tmp = sfmt_genrand_uint32(&sfmt) % (now->Ly - now->above_room->Ly -
                                                now->above_room->height - 2) +
                  1;
        for (i = now->above_room->Ly + now->above_room->height;
             i < (now->above_room->Ly + now->above_room->height + tmp); i++) {
          map[i][now->above_room->Lx + now->above_room->below_way] = 1;
        }
        for (i = (now->above_room->Ly + now->above_room->height + tmp);
             i < now->Ly; i++) {
          map[i][now->Lx + now->above_way] = 1;
        }
        for (i = min(now->above_room->Lx + now->above_room->below_way,
                     now->Lx + now->above_way);
             i <= max(now->above_room->Lx + now->above_room->below_way,
                      now->Lx + now->above_way);
             i++) {
          map[now->above_room->Ly + now->above_room->height + tmp][i] = 1;
        }
      } else {
        if (now->Ly + v - now->above_room->Ry < 3)
          v = now->Ly - now->above_room->Ry + 3;
        if (now->Lx + u - now->left_room->Rx < 3)
          u = now->Lx - now->left_room->Rx + 3;
        now->Lx += u;
        now->Ly += v;
        int tmp = sfmt_genrand_uint32(&sfmt) % (now->Lx - now->left_room->Lx -
                                                now->left_room->width - 2) +
                  1;
        for (i = now->left_room->Lx + now->left_room->width;
             i < (now->left_room->Lx + now->left_room->width + tmp); i++) {
          map[now->left_room->Ly + now->left_room->right_way][i] = 1;
        }
        for (i = (now->left_room->Lx + now->left_room->width + tmp);
             i < now->Lx; i++) {
          map[now->Ly + now->left_way][i] = 1;
        }
        for (i = min(now->left_room->Ly + now->left_room->right_way,
                     now->Ly + now->left_way);
             i <= max(now->left_room->Ly + now->left_room->right_way,
                      now->Ly + now->left_way);
             i++) {
          map[i][now->left_room->Lx + now->left_room->width + tmp] = 1;
        }
        tmp = sfmt_genrand_uint32(&sfmt) % (now->Ly - now->above_room->Ly -
                                            now->above_room->height - 2) +
              1;
        for (i = now->above_room->Ly + now->above_room->height;
             i < (now->above_room->Ly + now->above_room->height + tmp); i++) {
          map[i][now->above_room->Lx + now->above_room->below_way] = 1;
        }
        for (i = (now->above_room->Ly + now->above_room->height + tmp);
             i < now->Ly; i++) {
          map[i][now->Lx + now->above_way] = 1;
        }
        for (i = min(now->above_room->Lx + now->above_room->below_way,
                     now->Lx + now->above_way);
             i <= max(now->above_room->Lx + now->above_room->below_way,
                      now->Lx + now->above_way);
             i++) {
          map[now->above_room->Ly + now->above_room->height + tmp][i] = 1;
        }
      }
      for (k = now->Ly; k < now->Ly + now->height; k++)
        for (l = now->Lx; l < now->Lx + now->width; l++) {
          map[k][l] = 1;
        }
      if (enemy != NULL) {
        enemy->x = sfmt_genrand_uint32(&sfmt) % (now->width - 2) + now->Lx;
        enemy->y = sfmt_genrand_uint32(&sfmt) % (now->height - 2) + now->Ly;
        enemy = enemy->next;
      }
      if (enemy != NULL) {
        enemy->x = sfmt_genrand_uint32(&sfmt) % (now->width - 2) + now->Lx;
        enemy->y = sfmt_genrand_uint32(&sfmt) % (now->height - 2) + now->Ly;
        enemy = enemy->next;
      }
      if (cnt++ == goal) {
        int goal_x = sfmt_genrand_uint32(&sfmt) % (now->width - 2) + now->Lx;
        int goal_y = sfmt_genrand_uint32(&sfmt) % (now->height - 2) + now->Ly;
        map[goal_y][goal_x] = 2;
      }
      if (now->right_room != NULL) {
        now = now->right_room;
      } else {
        break;
      }
    }
    while (now->left_room != NULL) {
      now = now->left_room;
    }
    if (now->below_room != NULL) {
      now = now->below_room;
    } else {
      break;
    }
  }
}

int player_move(player *p) {
  int move = getchar();
  if (move == 'W' || move == 'w') {
    if (map[p->y - 1][p->x] == 0) {
      return 1;
    } else if (map[p->y - 1][p->x] == 2) {
      return 2;
    } else {
      p->y--;
    }
  } else if (move == 'A' || move == 'a') {
    if (map[p->y][p->x - 1] == 0) {
      return 1;
    } else if (map[p->y][p->x - 1] == 2) {
      return 2;
    } else {
      p->x--;
    }
  } else if (move == 'S' || move == 's') {
    if (map[p->y + 1][p->x] == 0) {
      return 1;
    } else if (map[p->y + 1][p->x] == 2) {
      return 2;
    } else {
      p->y++;
    }
  } else if (move == 'D' || move == 'd') {
    if (map[p->y][p->x + 1] == 0) {
      return 1;
    } else if (map[p->y][p->x + 1] == 2) {
      return 2;
    } else {
      p->x++;
    }
  } else if (move == 'K' || move == 'k') {
    Enemy *enemy = enemys->Start;
    while (enemy != NULL) {
      if (abs(enemy->x - p->x) <= 1 && abs(enemy->y - p->y) <= 1) {
        enemy->HP--;
      }
      if (enemy->HP <= 0) {
        if (enemy->prev != NULL) {
          enemy->prev->next = enemy->next;
        } else {
          enemys->Start = enemy->next;
        }
        if (enemy->next != NULL) {
          enemy->next->prev = enemy->prev;
        }
        Enemy *tmp = enemy;
        enemy = enemy->next;
        free(tmp);
      } else {
        enemy = enemy->next;
      }
    }
  } else {
    return 1;
  }
  return 0;
}

void enemy_move(player *p) {
  Enemy *now = enemys->Start;
  sfmt_t sfmt;
  sfmt_init_gen_rand(&sfmt, 41745);
  while (now != NULL) {
    if (abs(now->x - p->x) <= 1 && abs(now->y - p->y) <= 1) {
      p->HP--;
      if(p->HP <= 0) {
        printf("GAME OVER!!");
        exit(0);
      }
      now = now->next;
      continue;
    }
    int flg = 0;
    do {
      flg = 0;
      int move = sfmt_genrand_uint32(&sfmt) % 4;
      if (move == 0 && map[now->y][now->x - 1] == 1) {
        now->x = now->x - 1;
      } else if (move == 1 && map[now->y][now->x + 1] == 1) {
        now->x = now->x + 1;
      } else if (move == 2 && map[now->y - 1][now->x] == 1) {
        now->y = now->y - 1;
      } else if (move == 3 && map[now->y + 1][now->x] == 1) {
        now->y = now->y + 1;
      } else {
        flg = 1;
      }
    } while (flg);
    now = now->next;
  }
}

int main(void) {
  int seed;
  int i, floor = 0;
  sfmt_t sfmt;
  printf("SEED: ");
  scanf("%d", &seed);
  while (++floor) {
    Rooms *R = room_init();
    enemys = enemy_init();
    create_map(seed + floor - 1, R);
    player p = set_player(1, 1, 100, 1);
    Room *r = R->End;
    int x = r->Lx + r->width, y = r->Ly + r->height;
    while (r->above_room != NULL) {
      r = r->above_room;
      if (r->Lx + r->width > x) {
        x = r->Lx + r->width;
      }
    }
    r = R->End;
    while (r->left_room != NULL) {
      r = r->left_room;
      if (r->Ly + r->height > y) {
        y = r->Ly + r->height;
      }
    }
    print_map(x, y, seed, floor);
    while (1) {
      int acction;
      while (acction = player_move(&p)) {
        printf("\n");
        system("clear");
        view_map(x, y, &p);
        if (acction == 2) break;
      }
      if (acction == 2) break;
      enemy_move(&p);
    }
    free(map);
    free(enemys);
  }
}