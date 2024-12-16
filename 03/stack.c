#define DECLARE_STACK(TYPENAME, ELEMENT_TYPE)                                                     \
  typedef struct                                                                                  \
  {                                                                                               \
    size_t capacity;     /* スタックの容量 */                                              \
    size_t size;         /* スタックの現在のサイズ */                                  \
    ELEMENT_TYPE * data; /* スタックのデータ領域 */                                     \
  } TYPENAME;                                                                                     \
                                                                                                  \
  /* スタックの初期化 */                                                                  \
  static void TYPENAME##_init(TYPENAME * stack)                                                   \
  {                                                                                               \
    stack->capacity = 16;                                                                         \
    stack->size = 0;                                                                              \
    stack->data = (ELEMENT_TYPE *)malloc(sizeof(ELEMENT_TYPE) * stack->capacity);                 \
    if (!stack->data) {                                                                           \
      fprintf(stderr, "Failed to allocate memory for stack\n");                                   \
      exit(EXIT_FAILURE);                                                                         \
    }                                                                                             \
  }                                                                                               \
                                                                                                  \
  /* スタックに値をプッシュ */                                                         \
  static void TYPENAME##_push(TYPENAME * stack, ELEMENT_TYPE value)                               \
  {                                                                                               \
    if (stack->size >= stack->capacity) {                                                         \
      stack->capacity *= 2;                                                                       \
      stack->data = (ELEMENT_TYPE *)realloc(stack->data, sizeof(ELEMENT_TYPE) * stack->capacity); \
      if (!stack->data) {                                                                         \
        fprintf(stderr, "Failed to reallocate memory for stack\n");                               \
        exit(EXIT_FAILURE);                                                                       \
      }                                                                                           \
    }                                                                                             \
    stack->data[stack->size++] = value;                                                           \
  }                                                                                               \
                                                                                                  \
  /* スタックから値をポップ */                                                         \
  static ELEMENT_TYPE TYPENAME##_pop(TYPENAME * stack)                                            \
  {                                                                                               \
    if (stack->size == 0) {                                                                       \
      fprintf(stderr, "Stack underflow\n");                                                       \
      exit(EXIT_FAILURE);                                                                         \
    }                                                                                             \
    return stack->data[--stack->size];                                                            \
  }                                                                                               \
                                                                                                  \
  /* スタックのトップを参照（ポップしない） */                                 \
  static ELEMENT_TYPE TYPENAME##_peek(TYPENAME * stack)                                           \
  {                                                                                               \
    if (stack->size == 0) {                                                                       \
      fprintf(stderr, "Stack is empty\n");                                                        \
      exit(EXIT_FAILURE);                                                                         \
    }                                                                                             \
    return stack->data[stack->size - 1];                                                          \
  }                                                                                               \
                                                                                                  \
  /* スタックが空かどうかを判定 */                                                   \
  static int TYPENAME##_is_empty(TYPENAME * stack) { return stack->size == 0; }                   \
                                                                                                  \
  /* スタックのサイズを取得 */                                                         \
  static size_t TYPENAME##_size(TYPENAME * stack) { return stack->size; }                         \
                                                                                                  \
  /* スタックの解放 */                                                                     \
  static void TYPENAME##_destroy(TYPENAME * stack)                                                \
  {                                                                                               \
    free(stack->data);                                                                            \
    stack->data = NULL;                                                                           \
    stack->capacity = stack->size = 0;                                                            \
  }
