int g[2][3]; int main(){int a[2][3];int b[2][2][3];const int n=1;int i=n;a[i][2]=7;g[1][1]=a[i][2]+2;b[1][0][2]=g[1][1]+4;return a[1][2]+g[0][0]+g[1][1]+b[1][0][2];}
