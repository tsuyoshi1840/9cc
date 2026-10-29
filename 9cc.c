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
  int len;        // トークンの長さ
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

// 次が記号 op なら1つ進めて true。op は "+" や "==" のような文字列
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
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

void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
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

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

// 英字か _ なら true
bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// 英字か _ か数字なら true
bool is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

// p が q で始まっていれば true
bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
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

    // 2文字の記号（先にチェックする）
    if (startswith(p, "==") || startswith(p, "!=") ||
        startswith(p, "<=") || startswith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    // 1文字の記号
    if (strchr("+-*/()<>=;", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // 英字か _ で始まり、英数字か _ が続くものを変数名とする
    if (is_alpha(*p)) {
      char *start = p;
      while (is_alnum(*p))
        p++;
      cur = new_token(TK_IDENT, cur, start, p - start);
      continue;
    }

    if (isdigit(*p)) {
      char *start = p;
      cur = new_token(TK_NUM, cur, p, 0);
      cur->val = strtol(p, &p, 10);
      cur->len = p - start;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
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
  ND_EQ,     // ==
  ND_NE,     // !=
  ND_LT,     // <
  ND_LE,     // <=
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

// ローカル変数の一覧（連結リスト）
typedef struct LVar LVar;

struct LVar {
  LVar *next; // 次の変数
  char *name; // 変数名
  int len;    // 名前の長さ
  int offset; // rbpからの距離
};

LVar *locals; // 変数一覧の先頭

// 名前で変数を探す。見つからなければ NULL
LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

Node *code[100]; // 文（;で区切られたもの）を入れておく場所

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
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
  expect(";");
  return node;
}

// 式 = 代入
Node *expr() {
  return assign();
}

// 代入 = 等価 ("=" 代入)?
Node *assign() {
  Node *node = equality();
  if (consume("="))
    node = new_node(ND_ASSIGN, node, assign());
  return node;
}

// 等価 = 比較 ("==" 比較 | "!=" 比較)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

// 比較 = 足し算 ("<" 足し算 | "<=" 足し算 | ">" 足し算 | ">=" 足し算)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LT, node, add());
    else if (consume("<="))
      node = new_node(ND_LE, node, add());
    else if (consume(">"))
      node = new_node(ND_LT, add(), node); // a > b は b < a と同じ
    else if (consume(">="))
      node = new_node(ND_LE, add(), node); // a >= b は b <= a と同じ
    else
      return node;
  }
}

// 足し算 = 項 ("+" 項 | "-" 項)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// 項 = 単項 ("*" 単項 | "/" 単項)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// 単項 = ("+" | "-") 単項 | 因子
Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), unary()); // -x は 0-x
  return primary();
}

// 因子 = 数 | 変数 | "(" 式 ")"
Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    LVar *lvar = find_lvar(tok);
    if (!lvar) {
      // 初めて見る変数なら一覧に追加する
      lvar = calloc(1, sizeof(LVar));
      lvar->next = locals;
      lvar->name = tok->str;
      lvar->len = tok->len;
      lvar->offset = (locals ? locals->offset : 0) + 8;
      locals = lvar;
    }
    node->offset = lvar->offset;
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
  case ND_EQ: // 等しければ 1、違えば 0
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
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

  // 変数26個分（8バイト×26＝208）の場所をスタックに確保する
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
