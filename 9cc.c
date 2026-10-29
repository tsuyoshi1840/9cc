#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// トークナイザ
//

typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 変数名
  TK_NUM,      // 整数
  TK_EOF,      // 入力の終わり
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind; // トークンの種類
  Token *next;    // 次のトークン
  int val;        // 数のときの値
  char *str;      // トークンの文字列
};

Token *token;     // 今読んでいるトークン
char *user_input; // 入力全体

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラーの場所を ^ で示す
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  token = token->next;
  return true;
}

// 次が変数名なら、そのトークンを返して1つ進める
Token *consume_ident() {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *tok = token;
  token = token->next;
  return tok;
}

void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error_at(token->str, "'%c'ではありません", op);
  token = token->next;
}

int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strchr("+-*/()=;", *p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    // a〜z の1文字を変数名とする
    if ('a' <= *p && *p <= 'z') {
      cur = new_token(TK_IDENT, cur, p++);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

//
// パーサ（構文木を作る）
//

typedef enum {
  ND_ADD,    // +
  ND_SUB,    // -
  ND_MUL,    // *
  ND_DIV,    // /
  ND_ASSIGN, // =（代入）
  ND_LVAR,   // 変数
  ND_NUM,    // 整数
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind; // ノードの種類
  Node *lhs;     // 左の枝
  Node *rhs;     // 右の枝
  int val;       // 数のときの値
  int offset;    // 変数のときの、rbpからの距離
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *code[100]; // 文（;で区切られたもの）を入れておく場所

Node *stmt();
Node *expr();
Node *assign();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// プログラム = 文*
void program() {
  int i = 0;
  while (!at_eof())
    code[i++] = stmt();
  code[i] = NULL;
}

// 文 = 式 ";"
Node *stmt() {
  Node *node = expr();
  expect(';');
  return node;
}

// 式 = 代入
Node *expr() {
  return assign();
}

// 代入 = 足し算 ("=" 代入)?
Node *assign() {
  Node *node = add();
  if (consume('='))
    node = new_node(ND_ASSIGN, node, assign());
  return node;
}

// 足し算 = 項 ("+" 項 | "-" 項)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume('+'))
      node = new_node(ND_ADD, node, mul());
    else if (consume('-'))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// 項 = 単項 ("*" 単項 | "/" 単項)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume('*'))
      node = new_node(ND_MUL, node, unary());
    else if (consume('/'))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// 単項 = ("+" | "-") 単項 | 因子
Node *unary() {
  if (consume('+'))
    return unary();
  if (consume('-'))
    return new_node(ND_SUB, new_node_num(0), unary()); // -x は 0-x
  return primary();
}

// 因子 = 数 | 変数 | "(" 式 ")"
Node *primary() {
  if (consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->offset = (tok->str[0] - 'a' + 1) * 8; // a=8, b=16, ...
    return node;
  }

  return new_node_num(expect_number());
}

//
// コード生成（構文木 → アセンブリ）
//

// 変数の「住所」をスタックに積む
void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error("代入の左辺が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR: // 変数の値を読む
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN: // 変数に値を書き込む
    gen_lval(node->lhs);
    gen(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  default:
    break;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  default:
    break;
  }

  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }

  user_input = argv[1];
  token = tokenize(user_input);
  program();

  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // 変数26個分（a〜z）の場所をスタックに確保する
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");

  for (int i = 0; code[i]; i++) {
    gen(code[i]);
    printf("  pop rax\n"); // 文の結果を捨てる（最後の文の結果がraxに残る）
  }

  // 後片付けをして、最後の文の結果を返す
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
