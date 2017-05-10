#include <inc/lib.h>
struct UHPage{
	uint32 StartAddress;
	uint32 Size;
}Arr[(USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE];
uint32 UserHeapFirstFreeVirtualAddress = USER_HEAP_START;
bool Intial = 1;
void* FirstFit(uint32);
void* NextFit(uint32);
void* BestFit(uint32);
void* WorstFit(uint32);
void* malloc(uint32 size){
	uint32 Start;
	size = ROUNDUP(size, PAGE_SIZE);
	if (Intial){
		uint32 Start = USER_HEAP_START;
		while (Start < USER_HEAP_MAX){
			Arr[(Start - USER_HEAP_START) / PAGE_SIZE].StartAddress = Start;
			Arr[(Start - USER_HEAP_START) / PAGE_SIZE].Size = 0;
			Start += PAGE_SIZE;
		}
		Intial = 0;
	}
	if (sys_isUHeapPlacementStrategyNEXTFIT())Start = (uint32)NextFit(size);
	else if (sys_isUHeapPlacementStrategyBESTFIT())Start = (uint32)BestFit(size);
	else if (sys_isUHeapPlacementStrategyWORSTFIT())Start = (uint32)WorstFit(size);
	else Start = (uint32)FirstFit(size);
	if (!Start)return 0;
	sys_allocateMem(Start, size);
	uint32 tmpstart = Start;
	uint32 tmpsize = size;
	while (tmpsize > 0){
		Arr[(tmpstart - USER_HEAP_START) / PAGE_SIZE].StartAddress = Start;
		Arr[(tmpstart - USER_HEAP_START) / PAGE_SIZE].Size = size;
		tmpsize -= PAGE_SIZE;
		tmpstart += PAGE_SIZE;
	}
	UserHeapFirstFreeVirtualAddress = tmpstart;
	return (void*)Start;
}
void free(void* virtual_address){
	/*TODO: [PROJECT 2016 - Dynamic Deallocation] free() [User Side]*/
	uint32 VA = (uint32)virtual_address;
	uint32 size = Arr[((uint32)virtual_address - USER_HEAP_START) / PAGE_SIZE].Size;
	sys_freeMem(VA, size);
	while (size){
		Arr[(VA - USER_HEAP_START) / PAGE_SIZE].StartAddress = VA;
		Arr[(VA - USER_HEAP_START) / PAGE_SIZE].Size = 0;
		size -= PAGE_SIZE;
		VA += PAGE_SIZE;
	}
}
/*=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// realloc():
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.
//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().
//  Hint: you may need to use the sys_moveMem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		which switches to the kernel mode, calls moveMem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		in "memory_manager.c", then switch back to the user mode here
//	the moveMem function is empty, make sure to implement it.*/
void *realloc(void *virtual_address, uint32 new_size){
	/*TODO: [PROJECT 2016 - BONUS4] realloc() [User Side]
	// Write your code here, remove the panic and write your code*/
	panic("realloc() is not implemented yet...!!");
}
void *FirstFit(uint32 size){
	uint32 Start = USER_HEAP_START;
	uint32 FreeSize = 0;
	while (Start < USER_HEAP_MAX){
		if (!Arr[(Start - USER_HEAP_START) / PAGE_SIZE].Size)FreeSize += PAGE_SIZE;
		else FreeSize = 0;
		Start += PAGE_SIZE;
		if (FreeSize == size){
			Start -= size;
			break;
		}
	}
	if (FreeSize != size)return 0;
	return (void*)Start;
}
void *NextFit(uint32 size){
	uint32 Start = UserHeapFirstFreeVirtualAddress;
	uint32 FreeSize = 0;
	uint32 End = UserHeapFirstFreeVirtualAddress;
	while (Start < USER_HEAP_MAX){
		if (!Arr[(Start - USER_HEAP_START) / PAGE_SIZE].Size)FreeSize += PAGE_SIZE;
		else{
			FreeSize = 0;
			if (End == UserHeapFirstFreeVirtualAddress)End = Start;
		}
		Start += PAGE_SIZE;
		if (FreeSize == size){
			Start -= size;
			break;
		}
	}
	if (FreeSize != size){
		Start = USER_HEAP_START;
		FreeSize = 0;
		while (Start < End){
			if (!Arr[(Start - USER_HEAP_START) / PAGE_SIZE].Size)FreeSize += PAGE_SIZE;
			else FreeSize = 0;
			Start += PAGE_SIZE;
			if (FreeSize == size){
				Start -= size;
				break;
			}
		}
	}
	if (FreeSize != size)return 0;
	return (void*)Start;
}
void *BestFit(uint32 size){
	uint32 Start = USER_HEAP_START;
	uint32 FreeSize = 0;
	uint32 MinimumSize = USER_HEAP_MAX - USER_HEAP_START + 1;
	while (Start < USER_HEAP_MAX){
		if (!Arr[(Start - USER_HEAP_START) / PAGE_SIZE].Size){
			FreeSize += PAGE_SIZE;
			if (Start + PAGE_SIZE == USER_HEAP_MAX && FreeSize >= size && FreeSize < MinimumSize)MinimumSize = FreeSize;
		}else{
			if (FreeSize >= size && FreeSize < MinimumSize)MinimumSize = FreeSize;
			FreeSize = 0;
		}
		Start += PAGE_SIZE;
	}
	if (MinimumSize == USER_HEAP_MAX - USER_HEAP_START + 1)return 0;
	FreeSize = 0;
	Start = USER_HEAP_START;
	while (Start < USER_HEAP_MAX){
		if (!Arr[(Start - USER_HEAP_START) / PAGE_SIZE].Size){
			FreeSize += PAGE_SIZE;
			if (Start + PAGE_SIZE == USER_HEAP_MAX && FreeSize == MinimumSize){
				Start -= MinimumSize - PAGE_SIZE;
				break;
			}
		}else{
			if (FreeSize == MinimumSize){
				Start -= MinimumSize;
				break;
			}
			FreeSize = 0;
		}
		Start += PAGE_SIZE;
	}
	return (void*)Start;
}
void *WorstFit(uint32 size){
	uint32 Start = USER_HEAP_START;
	uint32 FreeSize = 0;
	uint32 MaximumSize = 0;
	while (Start < USER_HEAP_MAX){
		if (!Arr[(Start - USER_HEAP_START) / PAGE_SIZE].Size){
			FreeSize += PAGE_SIZE;
			if (Start + PAGE_SIZE == USER_HEAP_MAX && FreeSize >= size && FreeSize > MaximumSize)MaximumSize = FreeSize;
		}else{
			if (FreeSize >= size && FreeSize > MaximumSize)MaximumSize = FreeSize;
			FreeSize = 0;
		}
		Start += PAGE_SIZE;
	}
	if (!MaximumSize)return 0;
	FreeSize = 0;
	Start = USER_HEAP_START;
	while (Start < USER_HEAP_MAX){
		if (!Arr[(Start - USER_HEAP_START) / PAGE_SIZE].Size){
			FreeSize += PAGE_SIZE;
			if (Start + PAGE_SIZE == USER_HEAP_MAX && FreeSize == MaximumSize){
				Start -= MaximumSize - PAGE_SIZE;
				break;
			}
		}else{
			if (FreeSize == MaximumSize){
				Start -= MaximumSize;
				break;
			}
			FreeSize = 0;
		}
		Start += PAGE_SIZE;
	}
	return (void*)Start;
}
