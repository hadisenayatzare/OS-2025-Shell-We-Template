#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

int partition(int *arr,int l,int r){
    int pivot=arr[r],i=l-1;
    for(int j=l;j<r;j++){
        if(arr[j]<=pivot){
            i++; int t=arr[i]; arr[i]=arr[j]; arr[j]=t;
        }
    }
    int t=arr[i+1]; arr[i+1]=arr[r]; arr[r]=t;
    return i+1;
}

void quick_sort(int *arr,int l,int r){
    if(l>=r) return;
    int pi=partition(arr,l,r);
    pid_t pid=fork();
    if(pid==0){quick_sort(arr,l,pi-1); exit(0);}
    else{quick_sort(arr,pi+1,r); wait(NULL);}
}

int main(){
    int n;
    printf("Enter number of elements:\n");
    scanf("%d",&n);

    int *arr = mmap(NULL,n*sizeof(int),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);

    printf("Enter %d integers separated by spaces:\n", n);
    for(int i=0;i<n;i++) scanf("%d",&arr[i]);

    quick_sort(arr,0,n-1);

    printf("Sorted array:\n");
    for(int i=0;i<n;i++) printf("%d ",arr[i]);
    printf("\n");
    return 0;
}
