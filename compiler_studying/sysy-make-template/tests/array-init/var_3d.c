int g[2][3][4] = {1, 2, 3, 4, {5}, {6}, {7, 8}};
int main() {
  const int n = 2;
  int a[n][3][4] = {1, 2, 3, 4, {5}, {6}, {7, 8}};
  int sum = 0;
  int i = 0;
  while (i < 2) {
    int j = 0;
    while (j < 3) {
      int k = 0;
      while (k < 4) {
        sum = sum + g[i][j][k] + a[i][j][k];
        k = k + 1;
      }
      j = j + 1;
    }
    i = i + 1;
  }
  return sum;
}
