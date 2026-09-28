const int g[2][1][3] = {{1}, {2}};
int main() {
  const int n = 2;
  const int a[n][n] = {n + 1, n * 2, {5}};
  return g[0][0][0] + g[1][0][0] + g[1][0][2]
       + a[0][0] + a[0][1] + a[1][0] + a[1][1];
}
