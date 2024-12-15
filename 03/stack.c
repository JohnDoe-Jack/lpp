#define STACK_SIZE 16
#define NEW_STACK(T, ROOT)                                                      \
  (ROOT) = malloc(sizeof((T) * STACK_SIZE)) for (size_t i; i < STACK_SIZE; i++) \
  {                                                                             \
    (ROOT)[i] = NULL;                                                           \
  }
#define PUSH(STACK, NODE)                   \
  for (size_t i = 0; i < STACK_SIZE; i++) { \
    if ((STACK)[i] != NULL) continue;       \
    (STACK)[i] = NODE;                      \
  }

#define POP(STACK)                          \
  for (size_t i = 0; i < STACK_SIZE; i++) { \
    if () }