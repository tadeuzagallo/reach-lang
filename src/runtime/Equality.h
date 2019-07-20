#include "Value.h"

class Cell;

template<typename T>
bool isEqual(T, T);

template<>
bool isEqual(Cell*, Cell*);
