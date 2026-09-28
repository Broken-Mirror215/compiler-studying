int elem(int a[]) { return a[1]; }
int row(int a[][3]) { return elem(a[1]); }
int forward(int a[][3]) { return row(a); }
int main() {
  int b[2][3] = {{1, 2, 3}, {4, 5, 6}};
  return forward(b) + elem(b[0]);
}
