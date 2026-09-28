const int g[2][3] = {{1,2,3},{4,5,6}};
int elem(int a[]) { return a[2]; }
int whole(int a[][3]) { return elem(a[1]); }
int main() {
  const int local[2][3] = {{7,8,9},{10,11,12}};
  return whole(g)+elem(g[0])+whole(local)+elem(local[0]);
}
