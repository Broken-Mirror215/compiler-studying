int g[2][3];
void set(int a[]) { a[1]=9; }
void forward(int a[][3]) { set(a[1]); }
int main() {
  int local[2][3]={};
  forward(local);
  forward(g);
  return local[1][1]+g[1][1];
}
