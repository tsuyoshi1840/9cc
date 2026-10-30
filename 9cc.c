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

// キーワード（変数名に使えない予約語）なら true
bool is_keyword(char *p, int len) {
  char *kw[] = {"return", "if", "else", "while", "for"};
  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
    if (strlen(kw[i]) == len && !memcmp(p, kw[i], len))
      return true;
  return false;
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
    if (strchr("+-*/()<>=;{},&", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // 英字か _ で始まり、英数字か _ が続くもの（キーワードか変数名）
    if (is_alpha(*p)) {
      char *start = p;
      while (is_alnum(*p))
        p++;
      int len = p - start;
      if (is_keyword(start, len))
        cur = new_token(TK_RESERVED, cur, start, len); // キーワードは記号と同じ扱い
      else
        cur = new_token(TK_IDENT, cur, start, len);
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
  ND_RETURN, // return
  ND_IF,     // if
  ND_WHILE,  // while
  ND_FOR,    // for
  ND_BLOCK,  // { ... }
  ND_FUNCALL, // 関数呼び出し
  ND_ADDR,    // &（住所を取り出す）
  ND_DEREF,   // *（住所の中身を読む）
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind; // ノードの種類
  Node *lhs;     // 左の枝
  Node *rhs;     // 右の枝
  int val;       // 数のときの値
  int offset;    // 変数のときの、rbpからの距離

  // if / while / for で使う
  Node *cond; // 条件
  Node *then; // 条件が真のときに実行する文
  Node *els;  // else のときに実行する文
  Node *init; // for の初期化
  Node *inc;  // for の更新
  Node *body; // ブロックの中の最初の文
  Node *next; // 次の文
  char *funcname; // 関数名
  int funclen;    // 関数名の長さ
  Node *args;     // 引数（next でつながる）
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

// Node *code[100]; 

Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// 関数1つ分の情報
typedef struct Function Function;

struct Function {
  char *name;      // 関数名
  int len;         // 関数名の長さ
  Node *params[6]; // 引数（変数として扱う）
  int nparams;     // 引数の個数
  Node *body;      // 中身（{ } のブロック）
  int stack_size;  // 変数のために確保するスタックの大きさ
};

Function *code[100]; // 関数を入れておく場所

// 関数 = 名前 "(" (名前 ("," 名前)*)? ")" "{" 文* "}"
Function *function() {
  Token *tok = consume_ident();
  if (!tok)
    error_at(token->str, "関数名ではありません");

  Function *fn = calloc(1, sizeof(Function));
  fn->name = tok->str;
  fn->len = tok->len;

  locals = NULL; // 変数一覧は関数ごとにリセット
  expect("(");
  while (!consume(")")) {
    if (fn->nparams > 0)
      expect(",");
    fn->params[fn->nparams++] = primary(); // 引数を変数として登録
  }
  fn->body = stmt();
  fn->stack_size = locals ? locals->offset : 0;
  return fn;
}

// プログラム = 関数*
void program() {
  int i = 0;
  while (!at_eof())
    code[i++] = function();
  code[i] = NULL;
}

// 文 = 式 ";"
//    | "return" 式 ";"
//    | "if" "(" 式 ")" 文 ("else" 文)?
//    | "while" "(" 式 ")" 文
//    | "for" "(" 式? ";" 式? ";" 式? ")" 文
Node *stmt() {
  Node *node;
    if (consume("{")) {
    Node head = {0};
    Node *cur = &head;
    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }
    node = new_node(ND_BLOCK, NULL, NULL);
    node->body = head.next;
    return node;
  }

  if (consume("return")) {
    node = new_node(ND_RETURN, expr(), NULL);
    expect(";");
    return node;
  }

  if (consume("if")) {
    node = new_node(ND_IF, NULL, NULL);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }

  if (consume("while")) {
    node = new_node(ND_WHILE, NULL, NULL);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  if (consume("for")) {
    node = new_node(ND_FOR, NULL, NULL);
    expect("(");
    if (!consume(";")) {
      node->init = expr();
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->inc = expr();
      expect(")");
    }
    node->then = stmt();
    return node;
  }

  node = expr();
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
  if (consume("&"))
    return new_node(ND_ADDR, unary(), NULL);
  if (consume("*"))
    return new_node(ND_DEREF, unary(), NULL);
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
    if (tok && consume("(")) {
    Node *node = new_node(ND_FUNCALL, NULL, NULL);
    node->funcname = tok->str;
    node->funclen = tok->len;
    Node head = {0};
    Node *cur = &head;
    while (!consume(")")) {
      if (cur != &head)
        expect(",");
      cur->next = expr();
      cur = cur->next;
    }
    node->args = head.next;
    return node;
  }
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
void gen(Node *node); // gen は下で定義するので、先に名前だけ宣言しておく
// 変数の「住所」をスタックに積む
void gen_lval(Node *node) {
    if (node->kind == ND_DEREF) { // *p = … のとき、p の中身が書き込み先の住所
    gen(node->lhs);
    return;
  }
  if (node->kind != ND_LVAR)
  error("代入の左辺が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

int label_count = 0;
char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen(Node *node) {
  switch (node->kind) {
      case ND_FUNCALL: {
    int nargs = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      gen(arg);
      nargs++;
    }
    for (int i = nargs - 1; i >= 0; i--)
      printf("  pop %s\n", argreg[i]);

    int c = label_count++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  jnz .Lcall%d\n", c);
    printf("  mov rax, 0\n");
    printf("  call %.*s\n", node->funclen, node->funcname);
    printf("  jmp .Lend%d\n", c);
    printf(".Lcall%d:\n", c);
    printf("  sub rsp, 8\n");
    printf("  mov rax, 0\n");
    printf("  call %.*s\n", node->funclen, node->funcname);
    printf("  add rsp, 8\n");
    printf(".Lend%d:\n", c);
    printf("  push rax\n");
    return;
  }
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
      case ND_ADDR: // 変数の住所そのものを積む
    gen_lval(node->lhs);
    return;
  case ND_DEREF: // 住所を計算して、その中身を読む
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
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



// 文のコードを出力する（スタックには何も残さない）
void gen_stmt(Node *node) {
  int c;

  switch (node->kind) {
  case ND_RETURN: // 値を rax に入れて、すぐに関数から戻る
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;

  case ND_IF:
    c = label_count++;
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lelse%d\n", c);      // 条件が 0（偽）なら else へジャンプ
    gen_stmt(node->then);
    printf("  jmp .Lend%d\n", c);      // then が終わったら最後へジャンプ
    printf(".Lelse%d:\n", c);
    if (node->els)
      gen_stmt(node->els);
    printf(".Lend%d:\n", c);
    return;

  case ND_WHILE:
    c = label_count++;
    printf(".Lbegin%d:\n", c);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", c);       // 条件が偽ならループを抜ける
    gen_stmt(node->then);
    printf("  jmp .Lbegin%d\n", c);    // 先頭に戻ってもう一度
    printf(".Lend%d:\n", c);
    return;

  case ND_FOR:
    c = label_count++;
    if (node->init) {
      gen(node->init);
      printf("  pop rax\n");
    }
    printf(".Lbegin%d:\n", c);
    if (node->cond) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%d\n", c);
    }
    gen_stmt(node->then);
    if (node->inc) {
      gen(node->inc);
      printf("  pop rax\n");
    }
    printf("  jmp .Lbegin%d\n", c);
    printf(".Lend%d:\n", c);
    return;

    case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return;

  default: // 式の文：計算して、結果を rax に取り出す
    gen(node);
    printf("  pop rax\n");
    return;
  }
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

  for (int i = 0; code[i]; i++) {
    Function *fn = code[i];
    printf(".globl %.*s\n", fn->len, fn->name);
    printf("%.*s:\n", fn->len, fn->name);

    // 変数の場所を確保する（16の倍数に切り上げ）
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", (fn->stack_size + 15) / 16 * 16);

    // レジスタで渡された引数を、変数の場所にしまう
    for (int j = 0; j < fn->nparams; j++)
      printf("  mov [rbp-%d], %s\n", fn->params[j]->offset, argreg[j]);

    gen_stmt(fn->body);

    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
  return 0;
}