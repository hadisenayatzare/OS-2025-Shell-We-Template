#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

void merge(int *arr,int l,int m,int r){
    int n1=m-l+1,n2=r-m;
    int L[n1],R[n2];
    for(int i=0;i<n1;i++) L[i]=arr[l+i];
    for(int i=0;i<n2;i++) R[i]=arr[m+1+i];
    int i=0,j=0,k=l;
    while(i<n1 && j<n2) arr[k++] = (L[i]<=R[j])?L[i++] : R[j++];
    while(i<n1) arr[k++]=L[i++];
    while(j<n2) arr[k++]=R[j++];
}

void merge_sort(int *arr,int l,int r){
    if(l>=r) return;
    int m=(l+r)/2;
    pid_t pid=fork();
    if(pid==0){merge_sort(arr,l,m); exit(0);}
    else{merge_sort(arr,m+1,r); wait(NULL);}
    merge(arr,l,m,r);
}

int main(){
    int n;
    printf("Enter number of elements:\n");
    scanf("%d",&n);
    int *arr=mmap(NULL,n*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    printf("Enter %d integers separated by spaces:\n", n);
    for(int i=0;i<n;i++) scanf("%d",&arr[i]);
    merge_sort(arr,0,n-1);
    printf("Sorted array:\n");
    for(int i=0;i<n;i++) printf("%d ",arr[i]);
    printf("\n");
    return 0;
}
