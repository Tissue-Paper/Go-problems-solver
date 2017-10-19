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

void init_hashcode() {    //�Ֆʂ̏�Ԃ��n�b�V���R�[�h������֐�
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

    printf("���W����� >");
    scanf("%d-%d", &x, &y);

    switch (put_stone(WIDTH*y + x, turn)) {
    case 1: print_board();  break;
    case -1: printf("�����ɐ΂�����̂őłĂ܂���\n");   break;
    case -2: printf("���R�E�Ȃ̂ł����Ɏ��Ԃ��܂���\n"); break;
    case -3: printf("������֎~�_�ł�\n");          break;
    }
  }
  return 0;
}


void board_initialization() {    //��Քz��̏��������s���֐�
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
      if (x == 1 && y == 1)                printf("��");
      else if (x == B_SIZE && y == 1)      printf("��");
      else if (x == 1 && y == B_SIZE)      printf("��");
      else if (x == B_SIZE && y == B_SIZE) printf("��");
      else if (x == 1)                     printf("��");
      else if (y == 1)                     printf("��");
      else if (x == B_SIZE)                printf("��");
      else if (y == B_SIZE)                printf("��");
      else                                 printf("��");
      else if (board[z] == 1)              printf("��");
      else if (board[z] == 2)              printf("��");
    }
    printf("\n");
  }
  printf("�A�Q�n�}(���F%d  ���F%d)  ", prisoners[BLACK], prisoners[WHITE]);
}


// ������s���֐�
int put_stone(int z, int color) {
  int prisoner = 0, liberty, possibility_of_ko = 1, provisional_ko_z, new_z, i;

  //z��1�Ȃ̂Ńp�X
  if (z == 1) {
    turn = 3 - turn;
    ko_z = 0;
    return 1;
  }
  if (board[z] != SPACE) return -1; //���ɐ΂�����̂őłĂȂ�
  if (z == ko_z) return -2;         //�R�E�Ȃ̂őłĂȂ�

  board[z] = color;  //�ꎞ�I�Ȓ���

  //�㉺���E��T������
  for (i = 0; i<4; i++) {
    new_z = z + dir4[i];
    if (board[new_z] == color) possibility_of_ko = 0; //���������̐΂Ȃ�A�R�E�̉\�����Ȃ�
    else if (board[new_z] == 3 - color) {             //��������̐΂Ȃ��
      if (count_riberty(new_z, 3 - color, 1) == 0) {  //�_���̐��𐔂��āA�����_�����Ȃ��Ȃ�
        prisoner += take_stone(new_z, 3 - color, 1);  //�΂���菜���������s���A���̐ΐ����L�^����
        provisional_ko_z = new_z;                     //�R�E��������Ȃ��̂ō��W��ۑ�
      }
    }
  }

  liberty = count_riberty(z, color, 1);
  if (liberty == 0) { board[z] = SPACE; return -3; }    //����֎~�_�������ꍇ

  if (prisoner == 1 && possibility_of_ko && liberty == 1) ko_z = provisional_ko_z;  //�R�E�̌`�̏ꍇ
  else ko_z = 0;

  hashcode ^= hashboard[z][color];
  prisoners[turn] += prisoner;
  turn = 3 - turn;

  return 1;
}


int count_riberty(int z, int color, int first_time) {    //�_���̐��Ƃ��̍��W���L�^���A���̌���Ԃ��֐�
  static int liberty;
  int new_z, i, j;

  if (first_time) {
    liberty = 0;
    for (i = 0; i<BOARD_MAX; i++) researched[i] = place_riberty[i] = 0;
  }

  researched[z] = 1;
  researched_stone[z] = 1;  //atari�֐��p
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


int take_stone(int z, int color, int first_time) {  //�΂���菜���A���̌���Ԃ��֐�
  static int stone;
  int i;

  if (first_time) stone = 0;
  stone++;
  board[z] = SPACE;
  hashcode ^= hashboard[z][color];
  for (i = 0; i<4; i++) if (board[z + dir4[i]] == color) take_stone(z + dir4[i], color, 0);

  return stone;
}


int whether_existence() {    //��������̑Ώۂ����ׂđ��݂���Ȃ�1���A����Ă�����0��Ԃ��֐�
  int i;

  for (i = 0; target_place[i] != 0; i++)
    if (board[target_place[i]] != target_color) return 0;

  return 1;
}


int atari_target() {    //��������̑Ώۂ��A�^���Ȃ�1���A�Ⴄ�Ȃ�0��Ԃ��֐�
  int i;

  for (i = 0; target_place[i] != 0; i++)
    if (count_riberty(target_place[i], target_color, 1) == 1) return 1;

  return 0;
}


int atari() {    //�Տ�ɃA�^���ɂȂ��Ă��鑊��̐΂�����΁A���̃_�����W��z��ɕۑ����A���̌���Ԃ��֐�
  int z, i;

  for (i = 0; i<BOARD_MAX; i++) researched_stone[i] = place_atari[i] = 0;

  i = 0;
  for (z = WIDTH * B_SIZE + B_SIZE; z > WIDTH; z--)       //��ՉE�����獶���1-1�܂ł𒲂ׂ�
    if (board[z] == 3 - turn && researched_stone[z] == 0) //��������������̐΂ł���A���A�񒲍��ς݂ł���Ȃ�
      if (count_riberty(z, 3 - turn, 1) == 1) { place_atari[i++] = place_riberty[0]; }  //�_���̐��𐔂��Ă����P�Ȃ�΁A���̍��W�� place_atari �z��ɒǉ�����B

  return i;
}


int whether_its_eye(int z, int color) {  //��������ł��邩�ǂ�����Ԃ��֐��i�ȈՁj
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


int nigan_pro() {    //��������̑ΏۂɊ��S�ȂQ�Ⴊ����΂P�A�Ȃ���΂O��Ԃ��֐�
  int i;

  count_riberty(target_place[0], target_color, 1); //�_���̍��W�̒���
  for (i = 0; i<BOARD_MAX; i++) researched[i] = 0; //�����ςݔz��̏�����
  count_eye = 0;                                   //��̌��̏�����

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

// ��������ł��邩�ǂ�����Ԃ��֐��i�{�i�j
int whether_its_eye_pro(int z, int color) {
  int each_number[4] = { 0 };       //��_�A���΁A���΁A�ՊO�̐�
  int next_z = 0, next_direction;   //�ׂ̍��W�ƕ���
  int location;                     //���A�ӁA�����̂ǂ��ł��邩
  int myself, ret_num[4] = { 0 };   //�������g�̏��ƁA�߂�l���L�^����
  int new_z, i, r = 0;

  researched[z] = 1;
  for (i = 0; i < 4; i++) {      //�㉺���E�ɂ���A�����̐΂ƔՊO�̐��𐔂���
    new_z = z + dir4[i];
    if (board[new_z] == color) each_number[color]++;
    else if (board[new_z] == OUT) each_number[OUT]++;
    else { next_z = new_z; next_direction = i; }
  }

  if (each_number[color] + each_number[OUT] == 4) { //���̍��v���S�Ȃ�A�P��̉\������
    each_number[color] = 0;
    for (i = 0; i < 4; i++) naname(z + dir4_2[i]);  //�΂߂S�����𒲂ׂ�

    switch (each_number[OUT]) {
    case 2:  location = 0; break;  //���ł���
    case 1:  location = 1; break;  //�ӂł���
    case 0:  location = 2; break;  //�����ł���
    }
  }
  else if (each_number[color] + each_number[OUT] == 3) {        //���v���R�Ȃ�A����̉\������
    researched[next_z] = 1;
    for (i = 0; i<4; i++) each_number[board[next_z + dir4[i]]]++;  //�ׂ���ɂ����㉺���E�����ׂ�
    if (each_number[color] + each_number[OUT] != 6) return NOT_EYE;

    each_number[color] = 0;
    switch (next_direction) {    //�΂߂S�����𒲂ׂ�i�ǂ��ɗׂ̐΂����������ŏꍇ�����j
    case 0:  naname(z + dir4_2[2]);  naname(z + dir4_2[3]);  naname(next_z + dir4_2[0]);  naname(next_z + dir4_2[1]);  break;
    case 1:  naname(z + dir4_2[0]);  naname(z + dir4_2[1]);  naname(next_z + dir4_2[2]);  naname(next_z + dir4_2[3]);  break;
    case 2:  naname(z + dir4_2[1]);  naname(z + dir4_2[3]);  naname(next_z + dir4_2[0]);  naname(next_z + dir4_2[2]);  break;
    case 3:  naname(z + dir4_2[0]);  naname(z + dir4_2[2]);  naname(next_z + dir4_2[1]);  naname(next_z + dir4_2[3]);  break;
    }

    switch (each_number[OUT]) {
    case 3:  location = 0; break; //���ł���
    case 2:
    case 1: location = 1; break;  //�ӂł���
    case 0:  location = 2; break; //�����ł���
    }
  }
  else { return NOT_EYE; }

  switch (location) {    //���g�̊��]���i���A�ӁA�����ŏꍇ�����j
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

  for (i = 0; i<r; i++) {    //�߂�l�̌����A��r���s���i���̎��A���g�͕K���R��̊�̂ǂꂩ�ł���A�߂�l�͂����NOT_EYE������������̂ł���j
    if (ret_num[i] == NOT_EYE) continue;
    if (myself == MAYBE_EYE_1 && ret_num[i] == MAYBE_EYE_2) continue;
    if (myself == MAYBE_EYE_2 && (ret_num[i] == MAYBE_EYE_1 || ret_num[i] == MAYBE_EYE_2)) continue;
    return TWO_EYES;
  }

  return myself;
}


int kyusyo(int z) {    //�㉺���E�ɂ���Atarget_stone��OUT�̍��v����Ԃ�
  int sum = 0, new_z, i;

  for (i = 0; i<4; i++) {
    new_z = z + dir4[i];
    if (board[new_z] == target_color || board[new_z] == OUT) sum++;
  }
  return sum;
}


int make_choices(int tekazu) {        //���胊�X�g�̍쐬���s���֐�
  int coppy_canput[KOUHO], atari_no_kazu, z_index;
  int i, j, c;

  atari_no_kazu = atari();                                  //�`�z��i�A�^�����W���X�g�j�̍쐬
  memcpy(coppy_canput, canput[turn], sizeof(coppy_canput)); //�a�z��i���胊�X�g�j�̃R�s�[
  for (i = 0; i<KOUHO; i++) choices_data[tekazu][i] = 0;    //�b�z��i�ŏI�I�Ȍ��胊�X�g�j��0�ŏ�����

  for (i = 0; i<atari_no_kazu; i++)
    for (j = 0; j<KOUHO; j++)
      if (place_atari[i] == coppy_canput[j]) { coppy_canput[j] = 0; break; } //�`�z��Əd������a�z��̗v�f���폜

  c = 0;
  if (turn == target_color) choices_data[tekazu][c++] = 1;    //�����Ώې΂̎�ԂȂ�A�p�X��I�����ɒǉ�����

  //�a�z��̗v�f���P�����ׂ�
  for (i = 0; i<KOUHO; i++) {
    z_index = coppy_canput[i];
    if (board[z_index] == SPACE) {               //���̒n�_����_�Ȃ�
      board[z_index] = turn;                     //���߂��ɐ΂�u���Ă݂�
      switch (count_riberty(z_index, turn, 1)) { //�_���̐��𐔂���
      case 0:
        coppy_canput[i] = 0;                  //����0�Ȃ璅��֎~�_�Ȃ̂ō폜
        break;
      case 1:
        choices_data[tekazu][c++] = z_index;  //����1�Ȃ�b�z��̐擪�ɓ����B
        coppy_canput[i] = 0;                  //��납�璲�ׂĂ����΁A���Ԃ���񂵂ɂł���B
        break;
      }
      board[z_index] = SPACE;
    }
    else { coppy_canput[i] = 0; }      //���̒n�_�Ɋ��ɐ΂�����Ȃ�A����ł��Ȃ��̂ō폜
  }

  if (turn != target_color) {
    for (i = 0; i < KOUHO; i++) {
      if (coppy_canput[i] != 0 && whether_its_eye(coppy_canput[i], 3 - target_color)) {
        choices_data[tekazu][c++] = coppy_canput[i];
        coppy_canput[i] = 0;
      }
    }
  }

  //�a�z��̗v�f���Akyusyo�֐��̖߂�l���傫�����ɁA�b�z��Ɉڂ�
  for (i = 4; i>-1; i--)
    for (j = 0; j<KOUHO; j++)
      if (coppy_canput[j] != 0 && kyusyo(coppy_canput[j]) == i)
        choices_data[tekazu][c++] = coppy_canput[j];

  for (i = 0; i<atari_no_kazu; i++) if (place_atari[i] != ko_z)  //�R�E�̎��Ԃ��ł͂Ȃ��`�z��̗v�f���A�b�z��Ɉڂ�
    choices_data[tekazu][c++] = place_atari[i];

  return c;    //�ŏI�I�ɂb�z��Ɋi�[���ꂽ�v�f�̌���Ԃ�
}


int sikatsu_play_out(int tekazu_in_whole, int *tekazu_in_func) {  //�����̃v���C�A�E�g���s���֐�
  int next_index, i;

  next_index = tekazu_in_whole + *tekazu_in_func + 1;    //���ꂩ��łƂ��Ƃ��Ă���萔�i�C���f�b�N�X�j

  while (whether_existence()) {    //��������̑Ώۂ����݂���ԁA���ƍ������݂ɒ��肷��
    if (turn == 3 - target_color) {
      if (atari_target() != 0) return DEAD; //�Ώۂ��A�^���Ȃ�΁ADEAD��return
      if (nigan_pro()) return ALIVE;        //�ΏۂɂQ�Ⴊ����΁AALIVE��return
    }

    i = make_choices(next_index);                         //���胊�X�g�̍쐬
    while (1) {                                           //��₩��łĂ���T��
      if (i-- == 0) {                                     //�łĂ�ꏊ���Ȃ�������p�X
        if (kifu_data[next_index - 1] == 1) return ALIVE; //�p�X���Q�񑱂�����return
        put_stone(1, turn);
        break;
      }

      if (choices_data[next_index][i] != 1) {
        if (check_repeat_2(next_index, choices_data[next_index][i]) ||                  //�����̔����ł͂Ȃ���
          turn == target_color && whether_its_eye(choices_data[next_index][i], turn)) { //����Ԃ���ł͂Ȃ����ǂ����̃`�F�b�N
          choices_data[next_index][i] = 0;
          continue;
        }
      }

      kifu_data[next_index] = choices_data[next_index][i];
      turn_data[next_index] = turn;
      ko_data[next_index] = ko_z;
      memcpy(place_data[next_index], board, sizeof(board));  //�łꏊ�A��ԁA�R�E���A�Ֆʂ̏�Ԃ��L�^���A
      hash_data[next_index] = hashcode;
      put_stone(choices_data[next_index][i], turn); //������s��
      choices_data[next_index][i] = 0;              //��x�������Ƃ������ŁA���������
      (*tekazu_in_func)++; next_index++;            //�萔���C���N�������g
      //print_board();
      break;
    }
  }
  return DEAD;
}


int search_result(int tekazu_in_whole, int count) {      //�ؒT�����s���֐�
  int dead_or_alive, entrance, tekazu_in_func = 0, revel;
  int i;

  func_count++;
  //printf( "�����%d��ڂ�search_result�֐��B�c��萔��%d��\n", ++count, tekazu_in_whole ); print_board();

  dead_or_alive = sikatsu_play_out(tekazu_in_whole, &tekazu_in_func);  //��������������

  while (1) {
    for (; tekazu_in_func != 0; tekazu_in_func--) if (turn_data[tekazu_in_whole + tekazu_in_func] == 3 - dead_or_alive) break;
    if (tekazu_in_func == 0) return dead_or_alive;    //�֐����ŏ�̃��x�����Ȃ��Ȃ����̂�return

    revel = tekazu_in_whole + tekazu_in_func;
    for (i = 0; choices_data[revel][i] != 0; i++);    //���̃��x���ɂ��������̐�

    entrance = dead_or_alive;
    while (dead_or_alive == entrance) {
      if (i-- == 0) break;
      turn = turn_data[revel];
      ko_z = ko_data[revel];
      memcpy(board, place_data[revel], sizeof(board));  //�Ō�ɑI���ł���ǖʂɖ߂�
      hashcode = hash_data[revel];

      if (choices_data[revel][i] != 1) {
        if (check_repeat_2(revel, choices_data[revel][i])) continue;
        if (turn == target_color && whether_its_eye(choices_data[revel][i], turn)) continue;
      }

      put_stone(choices_data[revel][i], turn);
      kifu_data[revel] = choices_data[revel][i];      //������ύX����
      dead_or_alive = search_result(revel, count);    //���̏ꍇ�̌��ʂ��ǂ��Ȃ邩���m���߂�
    }
    tekazu_in_func--;
  }
}


int check_repeat(int revel) {    //�����ǖʂ��J��Ԃ��Ă��Ȃ����m�F����֐�
  int i;

  for (i = 1; i < revel; i++)
    if (hash_data[i] == hashcode) return 1;

  return 0;
}


int check_repeat_2(int revel, int wish_z) {    //����̋ǖʂŁA����̎��łƂ��Ƃ��Ă��Ȃ����m�F����֐�
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
      printf("\n%5s�n�_�̒T�����J�n...", z_to_xy(canput[BLACK][i]));
      if (search_result(0, 0) == DEAD)  can_kill[k++] = canput[BLACK][i];
      printf("�T���I��(�ω���=%7d)", func_count);
      henkasu += func_count;
    }
  }
  end = clock();
  time = (end - start) / CLOCKS_PER_SEC;
  printf("  ���ω���=%d  ����=%.2f\n", henkasu, time);

  memcpy(board, coppy_arr, sizeof(board));
  turn = BLACK;

  if (k == 0) {
    printf("�����������邱�Ƃ��ł��܂���ł���\n");
  }
  else if (k == 1) {
    printf("���̋ǖʂ͍��ԂȂ�A%s�ɑł��ł��B\n", z_to_xy(can_kill[0]));
  }
  else {
    printf("���̋ǖʂ͕����̎E����������܂��i");
    for (i = 0; i<k; i++) printf(" %s ", z_to_xy(can_kill[i]));
    printf("�j\n");
  }
  print_board();
}


int _z(int x, int y) {
  return WIDTH * y + x;
}


void sikatsu_initialization3() {  //�O�ڒ���̌`
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


void sikatsu_initialization5() {  //�ܖڒ���
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


void sikatsu_initialization10() {  //����ɃR�X�ދ�
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


void sikatsu_initialization11() {  //�p�ɒu���Đ؂�`
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


void sikatsu_initialization12() {    //�O�ڂ̋}���ɒu���`
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


void sikatsu_initialization13() {    //�ӂ̊p�ɒu���`
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


void sikatsu_initialization14() {    //�z�E���R��ŃI�N�`
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


void sikatsu_initialization15() {    //�؂��ē�����`
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