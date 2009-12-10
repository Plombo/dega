#define KMAP_1 0
#define KMAP_2 1
#define KMAP_UP 2
#define KMAP_DOWN 3
#define KMAP_LEFT 4
#define KMAP_RIGHT 5
#define KMAP_START 6

#define KMAP_PAUSE 7
#define KMAP_FRAMEADVANCE 8
#define KMAP_QUICKLOAD 9
#define KMAP_QUICKSAVE 10
#define KMAP_SPEEDUP 11
#define KMAP_SLOWDOWN 12
#define KMAP_FASTFORWARD 13

#define KMAP_SAVESLOT(x) (14+(x))

#define KMAP_AUTO_1 24
#define KMAP_AUTO_2 25
#define KMAP_AUTO_UP 26
#define KMAP_AUTO_DOWN 27
#define KMAP_AUTO_LEFT 28
#define KMAP_AUTO_RIGHT 29
#define KMAP_AUTO_START 30

#define KMAP_NORMALSPEED 31
#define KMAP_READONLY 32

#define KMAP_HOLD_1 33
#define KMAP_HOLD_2 34
#define KMAP_HOLD_UP 35
#define KMAP_HOLD_DOWN 36
#define KMAP_HOLD_LEFT 37
#define KMAP_HOLD_RIGHT 38
#define KMAP_HOLD_START 39

#define KMAP_P2_1 40
#define KMAP_P2_2 41
#define KMAP_P2_UP 42
#define KMAP_P2_DOWN 43
#define KMAP_P2_LEFT 44
#define KMAP_P2_RIGHT 45

#define KMAP_P2_AUTO_1 46
#define KMAP_P2_AUTO_2 47
#define KMAP_P2_AUTO_UP 48
#define KMAP_P2_AUTO_DOWN 49
#define KMAP_P2_AUTO_LEFT 50
#define KMAP_P2_AUTO_RIGHT 51

#define KMAP_P2_HOLD_1 52
#define KMAP_P2_HOLD_2 53
#define KMAP_P2_HOLD_UP 54
#define KMAP_P2_HOLD_DOWN 55
#define KMAP_P2_HOLD_LEFT 56
#define KMAP_P2_HOLD_RIGHT 57

#define KMAP_BUTTONSTATE 58
#define KMAP_FRAMECOUNTER 59

#define KMAP_SAVE_TO_SLOT(x) (60+(x))
#define KMAP_LOAD_FROM_SLOT(x) (70+(x))

#define KMAPCOUNT 80

#define ID_KMAP_START 41000
#define IS_ID_KMAP(id) ((id) >= ID_KMAP_START && (id) < ID_KMAP_START+KMAPCOUNT)

#define ID_KMAP(kmap) ((kmap) + ID_KMAP_START)
#define KMAP_ID(id) ((id) - ID_KMAP_START)

#define ID_CLEAR_KMAP_START 43000
#define IS_ID_CLEAR_KMAP(id) ((id) >= ID_CLEAR_KMAP_START && (id) < ID_CLEAR_KMAP_START+KMAPCOUNT)

#define ID_CLEAR_KMAP(kmap) ((kmap) + ID_CLEAR_KMAP_START)
#define KMAP_ID_CLEAR(id) ((id) - ID_CLEAR_KMAP_START)

#define ID_LAB_KMAP_START 42000
#define ID_LAB_KMAP(kmap) ((kmap) + ID_LAB_KMAP_START)

#define KMAP_DIALOG_RES(label, kmap, x, y) \
  LTEXT label, 0, x, y+3, 45, 10 \
  PUSHBUTTON "Set", ID_KMAP(kmap), x+45, y, 20, 15 \
  PUSHBUTTON "Clear", ID_CLEAR_KMAP(kmap), x+70, y, 20, 15 \
  LTEXT "", ID_LAB_KMAP(kmap), x+100, y+3, 20, 10
