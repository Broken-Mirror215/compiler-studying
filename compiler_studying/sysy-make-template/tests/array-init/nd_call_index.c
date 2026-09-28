int g[2][3][4];int id(int x){return x;}int main(){int a[2][20][30];a[1][19][29]=6;g[id(1)][id(2)][id(3)]=a[id(1)][id(19)][id(29)]+5;return g[1][2][3];}
