#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

#define B_SIZE    13
#define WIDTH     (B_SIZE + 2)
#define BOARD_MAX (WIDTH * WIDTH)
#define KOUHO     21

enum { SPACE, BLACK, WHITE, OUT };
enum { DEAD = 1, ALIVE };
enum { NOT_EYE, MAYBE_EYE_1, MAYBE_EYE_2, PARFECT_EYE, TWO_EYES };

int board[BOARD_MAX], researched[BOARD_MAX], researched_stone[BOARD_MAX];
int place_riberty[BOARD_MAX], place_atari[BOARD_MAX];
int kifu_data[70], turn_data[70], ko_data[70], place_data[70][BOARD_MAX], choices_data[70][KOUHO], canput[3][KOUHO];
int dir4[4] = { -WIDTH, WIDTH, -1, +1 }, dir4_2[4] = { (-WIDTH) - 1, (-WIDTH) + 1, (WIDTH)-1, (WIDTH)+1 };
int prisoners[3], ko_z;
int turn = BLACK;
int target_color = WHITE, target_place[5] = { 0 }, count_eye;
int func_count = 0;

void board_initialization(void);
void print_board(void);
int put_stone(int, int);
int count_riberty(int, int, int);
int take_stone(int, int, int);
int whether_existence(void);
int atari_target(void);
int atari(void);
int whether_its_eye(int, int);
int nigan_pro(void);
int whether_its_eye_pro(int, int);
int make_choices(int);
int sikatsu_play_out(int, int*);
int search_result(int, int);
int check_repeat(int);
int check_repeat_2(int, int);
void answer(void);
void sikatsu_initialization3();
void sikatsu_initialization5();
void sikatsu_initialization10();
void sikatsu_initialization11();
void sikatsu_initialization12();
void sikatsu_initialization13();
void sikatsu_initialization14();
void sikatsu_initialization15();
void sikatsu_initialization16();



void prt(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}
void send_gtp(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
}

#if defined(_MSC_VER)
typedef unsigned __int64 uint64;
#define PRIx64  "I64x"
#else
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
typedef uint64_t uint64;  // Linux
#define PRIx64  "llx"
#endif

#define HASH_KINDS 4      // 1...black, 2...white, 3...ko
#define HASH_KO 3
uint64 hashboard[BOARD_MAX][HASH_KINDS];
uint64 hashcode = 0;
uint64 hash_data[70];

unsigned long rand_xorshift128() {  // 2^128-1 
  static unsigned long x = 123456789, y = 362436069, z = 521288629, w = 88675123;
  unsigned long t;
  t = (x ^ (x << 11)) & 0xffffffff;
  x = y; y = z; z = w; return(w = (w ^ (w >> 19)) ^ (t ^ (t >> 8)));
}

uint64 rand64() {
  unsigned long r1 = rand_xorshift128();
  unsigned long r2 = rand_xorshift128();
  uint64 r = ((uint64)r1 << 32) | r2;
  return r;
}

void prt_code64(uint64 r) {
  prt("%08x%08x", (int)(r >> 32), (int)r);
};

void make_hashboard() {
  int z, i;
  for (z = 0; z<BOARD_MAX; z++) {
    for (i = 0; i<HASH_KINDS; i++) hashboard[z][i] = rand64();
  }
}

void init_hashcode() {    //盤面の状態をハッシュコード化する関数
  int i;

  for (i = 0; i < BOARD_MAX; i++) {
       if (board[i] == BLACK) hashcode ^= hashboard[i][BLACK];
    else if (board[i] == WHITE) hashcode ^= hashboard[i][WHITE];
  }
}

int main() {
  int x, y;
  board_initialization();
  sikatsu_initialization14();

  make_hashboard();
  init_hashcode();

  print_board();

  while (1) {
    if (turn == BLACK) answer();

    printf("座標を入力 >");
    scanf("%d-%d", &x, &y);

    switch (put_stone(WIDTH*y + x, turn)) {
    case 1: print_board();  break;
    case -1: printf("※既に石があるので打てません\n");   break;
    case -2: printf("※コウなのですぐに取り返せません\n"); break;
    case -3: printf("※着手禁止点です\n");          break;
    }
  }
  return 0;
}


void board_initialization() {    //碁盤配列の初期化を行う関数
  int x, y;

  for (y = 0; y<WIDTH; y++) {
  for (x = 0; x<WIDTH; x++) {
    if (x == 0 || y == 0 || x == WIDTH - 1 || y == WIDTH - 1) board[WIDTH*y + x] = OUT;
    else board[WIDTH*y + x] = SPACE;
  }
  }
}


void print_board() {
  int x, y, z;

  printf("\n  ");
  for (x = 1; x <= B_SIZE; x++) { if (x == 10) printf(" "); printf("%2d", x); }
  printf("\n");

  for (y = 1; y <= B_SIZE; y++) {
    printf("%2d ", y);
    for (x = 1; x <= B_SIZE; x++) {
      z = WIDTH * y + x;
      if (board[z] == 0)
      if (x == 1 && y == 1)                printf("┌");
      else if (x == B_SIZE && y == 1)      printf("┐");
      else if (x == 1 && y == B_SIZE)      printf("└");
      else if (x == B_SIZE && y == B_SIZE) printf("┘");
      else if (x == 1)                     printf("├");
      else if (y == 1)                     printf("┬");
      else if (x == B_SIZE)                printf("┤");
      else if (y == B_SIZE)                printf("┴");
      else                                 printf("┼");
      else if (board[z] == 1)              printf("○");
      else if (board[z] == 2)              printf("●");
    }
    printf("\n");
  }
  printf("アゲハマ(黒：%d  白：%d)  ", prisoners[BLACK], prisoners[WHITE]);
}


// 着手を行う関数
int put_stone(int z, int color) {
  int prisoner = 0, liberty, possibility_of_ko = 1, provisional_ko_z, new_z, i;

  //zが1なのでパス
  if (z == 1) {
    turn = 3 - turn;
    ko_z = 0;
    return 1;
  }
  if (board[z] != SPACE) return -1; //既に石があるので打てない
  if (z == ko_z) return -2;         //コウなので打てない

  board[z] = color;  //一時的な着手

  //上下左右を探索する
  for (i = 0; i<4; i++) {
    new_z = z + dir4[i];
    if (board[new_z] == color) possibility_of_ko = 0; //もし味方の石なら、コウの可能性がない
    else if (board[new_z] == 3 - color) {             //もし相手の石ならば
      if (count_riberty(new_z, 3 - color, 1) == 0) {  //ダメの数を数えて、もしダメがないなら
        prisoner += take_stone(new_z, 3 - color, 1);  //石を取り除く処理を行い、その石数を記録する
        provisional_ko_z = new_z;                     //コウかもしれないので座標を保存
      }
    }
  }

  liberty = count_riberty(z, color, 1);
  if (liberty == 0) { board[z] = SPACE; return -3; }    //着手禁止点だった場合

  if (prisoner == 1 && possibility_of_ko && liberty == 1) ko_z = provisional_ko_z;  //コウの形の場合
  else ko_z = 0;

  hashcode ^= hashboard[z][color];
  prisoners[turn] += prisoner;
  turn = 3 - turn;

  return 1;
}


int count_riberty(int z, int color, int first_time) {    //ダメの数とその座標を記録し、その個数を返す関数
  static int liberty;
  int new_z, i, j;

  if (first_time) {
    liberty = 0;
    for (i = 0; i<BOARD_MAX; i++) researched[i] = place_riberty[i] = 0;
  }

  researched[z] = 1;
  researched_stone[z] = 1;  //atari関数用
  for (i = 0; i<4; i++) {
    new_z = z + dir4[i];
    if (researched[new_z]) continue;
    if (board[new_z] == color) count_riberty(new_z, color, 0);
    else if (board[new_z] == SPACE) {
      researched[new_z] = 1;
      liberty++;
      for (j = 0; place_riberty[j] != 0; j++);
      place_riberty[j] = new_z;
    }
  }
  return liberty;
}


int take_stone(int z, int color, int first_time) {  //石を取り除き、その個数を返す関数
  static int stone;
  int i;

  if (first_time) stone = 0;
  stone++;
  board[z] = SPACE;
  hashcode ^= hashboard[z][color];
  for (i = 0; i<4; i++) if (board[z + dir4[i]] == color) take_stone(z + dir4[i], color, 0);

  return stone;
}


int whether_existence() {    //死活判定の対象がすべて存在するなら1を、取られていたら0を返す関数
  int i;

  for (i = 0; target_place[i] != 0; i++)
    if (board[target_place[i]] != target_color) return 0;

  return 1;
}


int atari_target() {    //死活判定の対象がアタリなら1を、違うなら0を返す関数
  int i;

  for (i = 0; target_place[i] != 0; i++)
    if (count_riberty(target_place[i], target_color, 1) == 1) return 1;

  return 0;
}


int atari() {    //盤上にアタリになっている相手の石があれば、そのダメ座標を配列に保存し、その個数を返す関数
  int z, i;

  for (i = 0; i<BOARD_MAX; i++) researched_stone[i] = place_atari[i] = 0;

  i = 0;
  for (z = WIDTH * B_SIZE + B_SIZE; z > WIDTH; z--)       //碁盤右下から左上の1-1までを調べる
    if (board[z] == 3 - turn && researched_stone[z] == 0) //もしそこが相手の石であり、かつ、非調査済みであるなら
      if (count_riberty(z, 3 - turn, 1) == 1) { place_atari[i++] = place_riberty[0]; }  //ダメの数を数えてもし１ならば、その座標を place_atari 配列に追加する。

  return i;
}


int whether_its_eye(int z, int color) {  //そこが眼であるかどうかを返す関数（簡易）
  int i, new_z;

  for (i = 0; i < 4; i++) {
    new_z = z + dir4[i];
    if (board[new_z] == SPACE || board[new_z] == 3 - color) return 0;
  }

  for (i = 0; i < 4; i++) {
    new_z = z + dir4[i];
    if (board[new_z] == color && (count_riberty(new_z, color, 1) == 1)) return 0;
  }
  return 1;
}


int nigan_pro() {    //死活判定の対象に完全な２眼があれば１、なければ０を返す関数
  int i;

  count_riberty(target_place[0], target_color, 1); //ダメの座標の調査
  for (i = 0; i<BOARD_MAX; i++) researched[i] = 0; //調査済み配列の初期化
  count_eye = 0;                                   //眼の個数の初期化

  for (i = 0; place_riberty[i] != 0; i++) {
    if (researched[place_riberty[i]] == 0)
      if (whether_its_eye_pro(place_riberty[i], target_color) == TWO_EYES) return 1;
  }
  return 0;
}


#define naname( new_z ) \
    do { \
      if( board[new_z] == color ) each_number[color]++; \
      else if( !researched[new_z] && ( board[new_z] != OUT ) ) \
        if( ( ret_num[r++] = whether_its_eye_pro(new_z, color) ) == TWO_EYES ) return TWO_EYES; \
    } while( 0 )

// そこが眼であるかどうかを返す関数（本格）
int whether_its_eye_pro(int z, int color) {
  int each_number[4] = { 0 };       //空点、黒石、白石、盤外の数
  int next_z = 0, next_direction;   //隣の座標と方向
  int location;                     //隅、辺、中央のどこであるか
  int myself, ret_num[4] = { 0 };   //自分自身の情報と、戻り値を記録する
  int new_z, i, r = 0;

  researched[z] = 1;
  for (i = 0; i < 4; i++) {      //上下左右にある、味方の石と盤外の数を数える
    new_z = z + dir4[i];
    if (board[new_z] == color) each_number[color]++;
    else if (board[new_z] == OUT) each_number[OUT]++;
    else { next_z = new_z; next_direction = i; }
  }

  if (each_number[color] + each_number[OUT] == 4) { //その合計が４つなら、単眼の可能性あり
    each_number[color] = 0;
    for (i = 0; i < 4; i++) naname(z + dir4_2[i]);  //斜め４方向を調べる

    switch (each_number[OUT]) {
    case 2:  location = 0; break;  //隅である
    case 1:  location = 1; break;  //辺である
    case 0:  location = 2; break;  //中央である
    }
  }
  else if (each_number[color] + each_number[OUT] == 3) {        //合計が３つなら、複眼の可能性あり
    researched[next_z] = 1;
    for (i = 0; i<4; i++) each_number[board[next_z + dir4[i]]]++;  //隣を基準にした上下左右も調べる
    if (each_number[color] + each_number[OUT] != 6) return NOT_EYE;

    each_number[color] = 0;
    switch (next_direction) {    //斜め４方向を調べる（どこに隣の石があったかで場合分け）
    case 0:  naname(z + dir4_2[2]);  naname(z + dir4_2[3]);  naname(next_z + dir4_2[0]);  naname(next_z + dir4_2[1]);  break;
    case 1:  naname(z + dir4_2[0]);  naname(z + dir4_2[1]);  naname(next_z + dir4_2[2]);  naname(next_z + dir4_2[3]);  break;
    case 2:  naname(z + dir4_2[1]);  naname(z + dir4_2[3]);  naname(next_z + dir4_2[0]);  naname(next_z + dir4_2[2]);  break;
    case 3:  naname(z + dir4_2[0]);  naname(z + dir4_2[2]);  naname(next_z + dir4_2[1]);  naname(next_z + dir4_2[3]);  break;
    }

    switch (each_number[OUT]) {
    case 3:  location = 0; break; //隅である
    case 2:
    case 1: location = 1; break;  //辺である
    case 0:  location = 2; break; //中央である
    }
  }
  else { return NOT_EYE; }

  switch (location) {    //自身の眼を評価（隅、辺、中央で場合分け）
  case 0:
    if (each_number[color] == 1) { myself = PARFECT_EYE; if (++count_eye == 2) return TWO_EYES; }
    else if (each_number[color] == 0) { if (next_z == 0) myself = MAYBE_EYE_1; else myself = MAYBE_EYE_2; }
    break;
  case 1:
    if (each_number[color] == 2) { myself = PARFECT_EYE; if (++count_eye == 2) return TWO_EYES; }
    else if (each_number[color] == 1) { if (next_z == 0) myself = MAYBE_EYE_1; else myself = MAYBE_EYE_2; }
    else return NOT_EYE;
    break;
  case 2:
    if (each_number[color] >= 3) { myself = PARFECT_EYE; if (++count_eye == 2) return TWO_EYES; }
    else if (each_number[color] == 2) { if (next_z == 0) myself = MAYBE_EYE_1; else myself = MAYBE_EYE_2; }
    else return NOT_EYE;
    break;
  }

  for (i = 0; i<r; i++) {    //戻り値の個数分、比較を行う（この時、自身は必ず３種の眼のどれかであり、戻り値はそれにNOT_EYEが加わったものである）
    if (ret_num[i] == NOT_EYE) continue;
    if (myself == MAYBE_EYE_1 && ret_num[i] == MAYBE_EYE_2) continue;
    if (myself == MAYBE_EYE_2 && (ret_num[i] == MAYBE_EYE_1 || ret_num[i] == MAYBE_EYE_2)) continue;
    return TWO_EYES;
  }

  return myself;
}


int kyusyo(int z) {    //上下左右にある、target_stoneとOUTの合計数を返す
  int sum = 0, new_z, i;

  for (i = 0; i<4; i++) {
    new_z = z + dir4[i];
    if (board[new_z] == target_color || board[new_z] == OUT) sum++;
  }
  return sum;
}


int make_choices(int tekazu) {        //候補手リストの作成を行う関数
  int coppy_canput[KOUHO], atari_no_kazu, z_index;
  int i, j, c;

  atari_no_kazu = atari();                                  //Ａ配列（アタリ座標リスト）の作成
  memcpy(coppy_canput, canput[turn], sizeof(coppy_canput)); //Ｂ配列（候補手リスト）のコピー
  for (i = 0; i<KOUHO; i++) choices_data[tekazu][i] = 0;    //Ｃ配列（最終的な候補手リスト）を0で初期化

  for (i = 0; i<atari_no_kazu; i++)
    for (j = 0; j<KOUHO; j++)
      if (place_atari[i] == coppy_canput[j]) { coppy_canput[j] = 0; break; } //Ａ配列と重複するＢ配列の要素を削除

  c = 0;
  if (turn == target_color) choices_data[tekazu][c++] = 1;    //もし対象石の手番なら、パスを選択肢に追加する

  //Ｂ配列の要素を１つずつ調べる
  for (i = 0; i<KOUHO; i++) {
    z_index = coppy_canput[i];
    if (board[z_index] == SPACE) {               //その地点が空点なら
      board[z_index] = turn;                     //ためしに石を置いてみて
      switch (count_riberty(z_index, turn, 1)) { //ダメの数を数える
      case 0:
        coppy_canput[i] = 0;                  //もし0なら着手禁止点なので削除
        break;
      case 1:
        choices_data[tekazu][c++] = z_index;  //もし1ならＣ配列の先頭に入れる。
        coppy_canput[i] = 0;                  //後ろから調べていけば、順番を後回しにできる。
        break;
      }
      board[z_index] = SPACE;
    }
    else { coppy_canput[i] = 0; }      //その地点に既に石があるなら、着手できないので削除
  }

  if (turn != target_color) {
    for (i = 0; i < KOUHO; i++) {
      if (coppy_canput[i] != 0 && whether_its_eye(coppy_canput[i], 3 - target_color)) {
        choices_data[tekazu][c++] = coppy_canput[i];
        coppy_canput[i] = 0;
      }
    }
  }

  //Ｂ配列の要素を、kyusyo関数の戻り値が大きい順に、Ｃ配列に移す
  for (i = 4; i>-1; i--)
    for (j = 0; j<KOUHO; j++)
      if (coppy_canput[j] != 0 && kyusyo(coppy_canput[j]) == i)
        choices_data[tekazu][c++] = coppy_canput[j];

  for (i = 0; i<atari_no_kazu; i++) if (place_atari[i] != ko_z)  //コウの取り返しではないＡ配列の要素を、Ｃ配列に移す
    choices_data[tekazu][c++] = place_atari[i];

  return c;    //最終的にＣ配列に格納された要素の個数を返す
}


int sikatsu_play_out(int tekazu_in_whole, int *tekazu_in_func) {  //死活のプレイアウトを行う関数
  int next_index, i;

  next_index = tekazu_in_whole + *tekazu_in_func + 1;    //これから打とうとしている手数（インデックス）

  while (whether_existence()) {    //死活判定の対象が存在する間、白と黒が交互に着手する
    if (turn == 3 - target_color) {
      if (atari_target() != 0) return DEAD; //対象がアタリならば、DEADをreturn
      if (nigan_pro()) return ALIVE;        //対象に２眼があれば、ALIVEをreturn
    }

    i = make_choices(next_index);                         //候補手リストの作成
    while (1) {                                           //候補から打てる手を探す
      if (i-- == 0) {                                     //打てる場所がなかったらパス
        if (kifu_data[next_index - 1] == 1) return ALIVE; //パスが２回続いたらreturn
        put_stone(1, turn);
        break;
      }

      if (choices_data[next_index][i] != 1) {
        if (check_repeat_2(next_index, choices_data[next_index][i]) ||                  //同一手の反復ではないか
          turn == target_color && whether_its_eye(choices_data[next_index][i], turn)) { //眼をつぶす手ではないかどうかのチェック
          choices_data[next_index][i] = 0;
          continue;
        }
      }

      kifu_data[next_index] = choices_data[next_index][i];
      turn_data[next_index] = turn;
      ko_data[next_index] = ko_z;
      memcpy(place_data[next_index], board, sizeof(board));  //打つ場所、手番、コウ情報、盤面の状態を記録し、
      hash_data[next_index] = hashcode;
      put_stone(choices_data[next_index][i], turn); //着手を行う
      choices_data[next_index][i] = 0;              //一度試したという事で、候補手を消す
      (*tekazu_in_func)++; next_index++;            //手数をインクリメント
      //print_board();
      break;
    }
  }
  return DEAD;
}


int search_result(int tekazu_in_whole, int count) {      //木探索を行う関数
  int dead_or_alive, entrance, tekazu_in_func = 0, revel;
  int i;

  func_count++;
  //printf( "これは%d代目のsearch_result関数。祖先手数は%d手\n", ++count, tekazu_in_whole ); print_board();

  dead_or_alive = sikatsu_play_out(tekazu_in_whole, &tekazu_in_func);  //勝った方が入る

  while (1) {
    for (; tekazu_in_func != 0; tekazu_in_func--) if (turn_data[tekazu_in_whole + tekazu_in_func] == 3 - dead_or_alive) break;
    if (tekazu_in_func == 0) return dead_or_alive;    //関数内で上のレベルがなくなったのでreturn

    revel = tekazu_in_whole + tekazu_in_func;
    for (i = 0; choices_data[revel][i] != 0; i++);    //そのレベルにおける候補手の数

    entrance = dead_or_alive;
    while (dead_or_alive == entrance) {
      if (i-- == 0) break;
      turn = turn_data[revel];
      ko_z = ko_data[revel];
      memcpy(board, place_data[revel], sizeof(board));  //最後に選択できる局面に戻す
      hashcode = hash_data[revel];

      if (choices_data[revel][i] != 1) {
        if (check_repeat_2(revel, choices_data[revel][i])) continue;
        if (turn == target_color && whether_its_eye(choices_data[revel][i], turn)) continue;
      }

      put_stone(choices_data[revel][i], turn);
      kifu_data[revel] = choices_data[revel][i];      //棋譜を変更する
      dead_or_alive = search_result(revel, count);    //その場合の結果がどうなるかを確かめる
    }
    tekazu_in_func--;
  }
}


int check_repeat(int revel) {    //同じ局面を繰り返していないか確認する関数
  int i;

  for (i = 1; i < revel; i++)
    if (hash_data[i] == hashcode) return 1;

  return 0;
}


int check_repeat_2(int revel, int wish_z) {    //同一の局面で、同一の手を打とうとしていないか確認する関数
  int i;

  for (i = revel - 1; i > 0; i--)
    if (kifu_data[i] == wish_z && turn_data[i] == turn)
      //if ( memcmp(place_data[i], board, sizeof(board)) == 0 ) return 1;
      if (hash_data[i] == hashcode) return 1;

  return 0;
}


char* z_to_xy(int z) {
  static char str[10] = { 0 };
  int x, y;

  x = z % WIDTH;
  y = z / WIDTH;

  sprintf(str, "%d-%d", x, y);
  return str;
}


void answer() {
  int coppy_arr[BOARD_MAX], can_kill[15] = { 0 }, i, k = 0;
  int henkasu = 0;
  clock_t start, end;
  double time;

  start = clock();

  memcpy(coppy_arr, board, sizeof(board));
  for (i = 0; canput[BLACK][i] != 0; i++) {
    func_count = 0;
    memcpy(board, coppy_arr, sizeof(board));
    turn = BLACK;
    if (put_stone(canput[BLACK][i], BLACK) == 1) {
      printf("\n%5s地点の探索を開始...", z_to_xy(canput[BLACK][i]));
      if (search_result(0, 0) == DEAD)  can_kill[k++] = canput[BLACK][i];
      printf("探索終了(変化数=%7d)", func_count);
      henkasu += func_count;
    }
  }
  end = clock();
  time = (end - start) / CLOCKS_PER_SEC;
  printf("  総変化数=%d  時間=%.2f\n", henkasu, time);

  memcpy(board, coppy_arr, sizeof(board));
  turn = BLACK;

  if (k == 0) {
    printf("答えを見つけることができませんでした\n");
  }
  else if (k == 1) {
    printf("この局面は黒番なら、%sに打つ一手です。\n", z_to_xy(can_kill[0]));
  }
  else {
    printf("この局面は複数の殺し方があります（");
    for (i = 0; i<k; i++) printf(" %s ", z_to_xy(can_kill[i]));
    printf("）\n");
  }
  print_board();
}


int _z(int x, int y) {
  return WIDTH * y + x;
}


void sikatsu_initialization3() {  //三目中手の形
  int haichi_b[20] = { _z(1,11), _z(2,11), _z(3,11), _z(4,11), _z(5,11), _z(5,12), _z(5,13) };
  int haichi_w[20] = { _z(1,12), _z(2,12), _z(3,12), _z(4,12), _z(4,13) };

  int canput_b[15] = { _z(1,13), _z(2,13), _z(3,13) };
  int canput_w[15] = { _z(1,13), _z(2,13), _z(3,13) };

  int target_stone[5] = { _z(1,12) }, i;

  for (i = 0; haichi_b[i] != 0; i++) board[haichi_b[i]] = BLACK;
  for (i = 0; haichi_w[i] != 0; i++) board[haichi_w[i]] = WHITE;
  for (i = 0; canput_b[i] != 0; i++) canput[1][i] = canput_b[i];
  for (i = 0; canput_w[i] != 0; i++) canput[2][i] = canput_w[i];
  for (i = 0; target_stone[i] != 0; i++) target_place[i] = target_stone[i];
}


void sikatsu_initialization5() {  //五目中手
  int haichi_b[20] = { _z(1,10), _z(2,10), _z(3,10), _z(4,10), _z(5,10), _z(5,11), _z(5,12), _z(5,13) };
  int haichi_w[20] = { _z(1,11), _z(2,11), _z(3,11), _z(4,11), _z(4,12), _z(4,13), _z(1,12) };

  int canput_b[15] = { _z(2,12), _z(3,12), _z(1,13), _z(2,13), _z(3,13) };
  int canput_w[15] = { _z(2,12), _z(3,12), _z(1,13), _z(2,13), _z(3,13) };

  int target_stone[5] = { _z(3,11) }, i;

  for (i = 0; haichi_b[i] != 0; i++) board[haichi_b[i]] = BLACK;
  for (i = 0; haichi_w[i] != 0; i++) board[haichi_w[i]] = WHITE;
  for (i = 0; canput_b[i] != 0; i++) canput[1][i] = canput_b[i];
  for (i = 0; canput_w[i] != 0; i++) canput[2][i] = canput_w[i];
  for (i = 0; target_stone[i] != 0; i++) target_place[i] = target_stone[i];
}


void sikatsu_initialization10() {  //一線にコスむ筋
  int haichi_b[20] = { _z(2,10), _z(3,10), _z(4,10), _z(5,10), _z(6,10), _z(2,11), _z(6,11), _z(6,12), _z(8,12) };
  int haichi_w[20] = { _z(1,12), _z(2,12), _z(3,11), _z(4,11), _z(5,11), _z(3,13) };

  int canput_b[15] = { _z(1,11), _z(3,12), _z(4,12), _z(5,12), _z(1,13), _z(2,13), _z(3,13), _z(4,13), _z(5,13), _z(6,13), _z(7,13) };
  int canput_w[15] = { _z(3,12), _z(4,12), _z(5,12), _z(1,13), _z(2,13), _z(3,13), _z(4,13), _z(5,13), _z(6,13) };

  int target_stone[5] = { _z(2,12), _z(3,11) }, i;

  for (i = 0; haichi_b[i] != 0; i++) board[haichi_b[i]] = BLACK;
  for (i = 0; haichi_w[i] != 0; i++) board[haichi_w[i]] = WHITE;
  for (i = 0; canput_b[i] != 0; i++) canput[1][i] = canput_b[i];
  for (i = 0; canput_w[i] != 0; i++) canput[2][i] = canput_w[i];
  for (i = 0; target_stone[i] != 0; i++) target_place[i] = target_stone[i];
}


void sikatsu_initialization11() {  //角に置いて切る形
  int haichi_b[20] = { _z(2,10), _z(3,10), _z(4,10), _z(5,10), _z(6,10), _z(2,11), _z(6,11), _z(6,12), _z(8,12) };
  int haichi_w[20] = { _z(1,12), _z(2,12), _z(3,11), _z(4,11), _z(5,11), _z(5,12) };

  int canput_b[15] = { _z(1,11), _z(3,12), _z(4,12), _z(1,13), _z(2,13), _z(3,13), _z(4,13), _z(5,13), _z(6,13), _z(7,13) };
  int canput_w[15] = { _z(3,12), _z(4,12), _z(1,13), _z(2,13), _z(3,13), _z(4,13), _z(5,13), _z(6,13) };

  int target_stone[5] = { _z(2,12), _z(3,11) }, i;

  for (i = 0; haichi_b[i] != 0; i++) board[haichi_b[i]] = BLACK;
  for (i = 0; haichi_w[i] != 0; i++) board[haichi_w[i]] = WHITE;
  for (i = 0; canput_b[i] != 0; i++) canput[1][i] = canput_b[i];
  for (i = 0; canput_w[i] != 0; i++) canput[2][i] = canput_w[i];
  for (i = 0; target_stone[i] != 0; i++) target_place[i] = target_stone[i];
}


void sikatsu_initialization12() {    //三目の急所に置く形
  int haichi_b[20] = { _z(2,10), _z(3,10), _z(4,10), _z(5,10), _z(6,10), _z(2,11), _z(6,11), _z(7,12), _z(8,11) };
  int haichi_w[20] = { _z(1,12), _z(2,12), _z(3,11), _z(4,11), _z(5,11), _z(6,12) };

  int canput_b[15] = { _z(1,11), _z(3,12), _z(4,12), _z(5,12), _z(6,12), _z(1,13), _z(2,13), _z(3,13), _z(4,13), _z(5,13), _z(6,13), _z(7,13), _z(8,13) };
  int canput_w[15] = { _z(3,12), _z(4,12), _z(5,12), _z(6,12), _z(1,13), _z(2,13), _z(3,13), _z(4,13), _z(5,13), _z(6,13), _z(7,13) };

  int target_stone[5] = { _z(2,12), _z(3,11) }, i;

  for (i = 0; haichi_b[i] != 0; i++) board[haichi_b[i]] = BLACK;
  for (i = 0; haichi_w[i] != 0; i++) board[haichi_w[i]] = WHITE;
  for (i = 0; canput_b[i] != 0; i++) canput[1][i] = canput_b[i];
  for (i = 0; canput_w[i] != 0; i++) canput[2][i] = canput_w[i];
  for (i = 0; target_stone[i] != 0; i++) target_place[i] = target_stone[i];
}


void sikatsu_initialization13() {    //辺の角に置く形
  int haichi_b[20] = { _z(3,10), _z(4,10), _z(5,10), _z(6,10), _z(7,10), _z(8,10), _z(9,10), _z(3,11), _z(9,11), _z(11,11), _z(3,12), _z(10,12) };
  int haichi_w[20] = { _z(4,11), _z(5,11), _z(6,11), _z(7,11), _z(8,11), _z(4,12), _z(8,12), _z(9,12), _z(3,13) };

  int canput_b[15] = { _z(2,13), _z(3,13), _z(4,13), _z(5,13), _z(6,13), _z(7,13), _z(8,13), _z(9,13), _z(10,13), _z(5,12), _z(6,12), _z(7,12), _z(11,13) };
  int canput_w[15] = { _z(3,13), _z(4,13), _z(5,13), _z(6,13), _z(7,13), _z(8,13), _z(9,13), _z(10,13), _z(5,12), _z(6,12), _z(7,12) };

  int target_stone[5] = { _z(4,12) }, i;

  for (i = 0; haichi_b[i] != 0; i++) board[haichi_b[i]] = BLACK;
  for (i = 0; haichi_w[i] != 0; i++) board[haichi_w[i]] = WHITE;
  for (i = 0; canput_b[i] != 0; i++) canput[1][i] = canput_b[i];
  for (i = 0; canput_w[i] != 0; i++) canput[2][i] = canput_w[i];
  for (i = 0; target_stone[i] != 0; i++) target_place[i] = target_stone[i];
}


void sikatsu_initialization14() {    //ホウリコんでオク形
  int haichi_b[20] = { _z(2,1), _z(2,2), _z(3,3), _z(3,4), _z(4,4), _z(5,4), _z(6,4), _z(7,5), _z(8,5), _z(9,4), _z(9,3), _z(9,2), _z(9,1) };
  int haichi_w[20] = { _z(3,2), _z(4,1), _z(4,3), _z(5,3), _z(6,3), _z(7,4), _z(8,3), _z(8,2), _z(8,1) };

  int canput_b[KOUHO] = { _z(3,1), _z(4,2), _z(5,1), _z(5,2), _z(6,1), _z(6,2), _z(7,1), _z(7,2), _z(7,3), _z(8,4), _z(3,2), _z(4,1), _z(7,4), _z(8,1), _z(8,2), _z(8,3) };
  int canput_w[KOUHO] = { _z(3,1), _z(4,2), _z(5,1), _z(5,2), _z(6,1), _z(6,2), _z(7,1), _z(7,2), _z(7,3), _z(8,4), _z(3,2), _z(4,1), _z(7,4) };

  int target_stone[5] = { _z(4,3) }, i;

  for (i = 0; haichi_b[i] != 0; i++) board[haichi_b[i]] = BLACK;
  for (i = 0; haichi_w[i] != 0; i++) board[haichi_w[i]] = WHITE;
  for (i = 0; canput_b[i] != 0; i++) canput[1][i] = canput_b[i];
  for (i = 0; canput_w[i] != 0; i++) canput[2][i] = canput_w[i];
  for (i = 0; target_stone[i] != 0; i++) target_place[i] = target_stone[i];
}


void sikatsu_initialization15() {    //切って逃げる形
  int haichi_b[20] = { _z(3,2), _z(4,2), _z(3,3), _z(2,4), _z(2,5), _z(3,6), _z(3,7), _z(4,7), _z(5,7), _z(6,7), _z(7,7), _z(8,7), _z(8,6), _z(7,2), _z(8,3), _z(9,2), _z(9,4), _z(9,5) };
  int haichi_w[20] = { _z(5,2), _z(4,3), _z(6,3), _z(5,4), _z(3,4), _z(3,5), _z(4,6), _z(5,6), _z(6,6), _z(7,6), _z(8,4), _z(8,5) };

  int canput_b[KOUHO] = { _z(5,1), _z(6,1), _z(6,2), _z(5,3), _z(7,3), _z(4,4), _z(6,4), _z(7,4), _z(4,5), _z(5,5), _z(6,5), _z(7,5), _z(5,2), _z(4,3), _z(6,3), _z(5,4), _z(3,4), _z(3,5), _z(8,4), _z(8,5) };
  int canput_w[KOUHO] = { _z(6,2), _z(5,3), _z(7,3), _z(4,4), _z(6,4), _z(7,4), _z(4,5), _z(5,5), _z(6,5), _z(7,5), _z(5,2), _z(4,3), _z(6,3), _z(5,4), _z(3,4), _z(3,5), _z(8,4), _z(8,5) };

  int target_stone[5] = { _z(4,6) }, i;

  for (i = 0; haichi_b[i] != 0; i++) board[haichi_b[i]] = BLACK;
  for (i = 0; haichi_w[i] != 0; i++) board[haichi_w[i]] = WHITE;
  for (i = 0; canput_b[i] != 0; i++) canput[1][i] = canput_b[i];
  for (i = 0; canput_w[i] != 0; i++) canput[2][i] = canput_w[i];
  for (i = 0; target_stone[i] != 0; i++) target_place[i] = target_stone[i];
}
