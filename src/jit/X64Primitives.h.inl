#pragma once

enum Opcode : uint8_t;
enum class ModRM : uint8_t;

void move(Opcode, Register, Offset);
void emitRex(uint8_t, Register, Register);
void emitModRm(ModRM, uint8_t, Register);
void emitModRm(ModRM, uint8_t, Register, Register);
void emitSib(uint8_t, Register, Register);
void emitOpcode(Opcode);
void emitOpcode(uint8_t, uint8_t);
