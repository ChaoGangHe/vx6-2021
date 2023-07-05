// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
#define NBUCKET 13

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


//创建哈希表，锁，哈希函数
//1.将缓存设置为长度为13的哈希表，并为每一个哈希表的键值设置一个哈希桶，且为每一个哈希桶设置一个锁
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  struct spinlock bucketlock[NBUCKET];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf bucket[NBUCKET];
} bcache;


//接下来解决一堆没那么多操作的函数

//2.初始化缓存时，将每一个桶的锁以及整一个缓存的锁都进行初始化。同时为每一个桶平均分配缓存，每一个桶都是一个循环双链表，
//块越在桶的前面表示块最近被使用过
void
binit(void)
{
  struct buf *b;

  
  initlock(&bcache.lock, "bcache");
  for(int i=0;i<NBUCKET;i++){
    initlock(&bcache.bucketlock[i],"bcache");
    bcache.bucket[i].next = &bcache.bucket[i];
    bcache.bucket[i].prev = &bcache.bucket[i];

  for(b = bcache.buf+NBUF/NBUCKET*i; b!=bcache.buf+NBUF/NBUCKET*(i+1); b++){
    b->next = bcache.bucket[i].next;
    b->next->prev = b;
    b->prev = &bcache.bucket[i];
    bcache.bucket[i].next = b;
    }
  
  }
  
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
// 3 在得到块时，如果缓存中有块，则直接获得该块。如果缓存中
//没有块，但是对应的哈希桶存在空闲的最久未使用的缓存块，则直接设置该缓存块。否则从其他的哈希桶中找出空闲的最久未使用的缓存块。
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int index = blockno%NBUCKET;
  acquire(&bcache.bucketlock[index]);

  // Is the block already cached?
  for(b = bcache.bucket[index].next; b!= &bcache.bucket[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucketlock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.bucket[index].prev; b != &bcache.bucket[index]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.bucketlock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucketlock[index]);
  acquire(&bcache.lock);
  for(int i=0;i<NBUCKET;i++){
    if(i==index)continue;
    acquire(&bcache.bucketlock[i]);
    for(b = bcache.bucket[i].prev;b!=&bcache.bucket[i];b=b->prev){
      if(b->refcnt == 0){
        acquire(&bcache.bucketlock[index]);
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.bucketlock[i]);
        b->next = &bcache.bucket[index];
        bcache.bucket[index].prev->next = b;
        b->prev = bcache.bucket[index].prev;
        bcache.bucket[index].prev=b;
        release(&bcache.bucketlock[index]);
        release(&bcache.lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.bucketlock[i]);
  }
  release(&bcache.lock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
//获取锁，然后将块引用计数-1，并不明白提升所说的不用锁的方法是什么
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int index = b->blockno%NBUCKET;
  acquire(&(bcache.bucketlock[index]));
  b->refcnt--;
  if(b->refcnt==0){
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.bucket[index].next;
    b->prev = &bcache.bucket[index];
    bcache.bucket[index].next->prev = b;
    bcache.bucket[index].next = b;
  }
  release(&(bcache.bucketlock[index]));
}

void
bpin(struct buf *b) {
  acquire(&(bcache.lock));
  b->refcnt++;
  release(&(bcache.lock));
}


void
bunpin(struct buf *b) {
  acquire(&(bcache.lock));
  b->refcnt--;
  release(&(bcache.lock));
}



