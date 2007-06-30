#define KMAP_1 0
#define KMAP_2 1
#define KMAP_UP 2
#define KMAP_DOWN 3
#define KMAP_LEFT 4
#define KMAP_RIGHT 5
#define KMAP_START 6

#define KMAP_PAUSE 7
#define KMAP_FRAMEADVANCE 8

#define KMAPCOUNT 9

#define ID_KMAP_START 41000
#define IS_ID_KMAP(id) ((id) >= ID_KMAP_START && (id) < ID_KMAP_START+KMAPCOUNT)

#define ID_KMAP(kmap) ((kmap) + ID_KMAP_START)
#define KMAP_ID(id) ((id) - ID_KMAP_START)

#define ID_LAB_KMAP_START 42000
#define ID_LAB_KMAP(kmap) ((kmap) + ID_LAB_KMAP_START)

#define KMAP_DIALOG_RES(label, kmap, x, y) \
  LTEXT label, 0, x, y+3, 50, 10 \
  PUSHBUTTON "Set", ID_KMAP(kmap), x+60, y, 30, 15 \
  LTEXT "", ID_LAB_KMAP(kmap), x+100, y+3, 20, 10
