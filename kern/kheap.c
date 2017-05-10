#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>
struct KHPage{
	uint32 StartAddress;
	uint32 Size;
}Arr[(KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE];
uint32 KernalHeapFirstFreeVirtualAddress = KERNEL_HEAP_START;
uint32 MapPhysicalToVirtual[1024 * 1024];
uint32 Fit = 0;
bool Initialize = 1;
void *ContinuousFit(uint32 size);
void *FirstFit(uint32 size);
void *NextFit(uint32 size);
void *BestFit(uint32 size);
void *WorstFit(uint32 size);
void* kmalloc(unsigned int size){
	size = ROUNDUP(size, PAGE_SIZE);
	if (Initialize){
		uint32 Start = KERNEL_HEAP_START;
		while (Start < KERNEL_HEAP_MAX){
			Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].StartAddress = Start;
			Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size = 0;
			Start += PAGE_SIZE;
		}
		Initialize = 0;
	}
	uint32 Start;
	if (Fit == 1)Start = (uint32)FirstFit(size);
	else if (Fit == 2)Start = (uint32)NextFit(size);
	else if (Fit == 3)Start = (uint32)BestFit(size);
	else if (Fit == 4)Start = (uint32)WorstFit(size);
	else Start = (uint32)ContinuousFit(size);
	if (!Start)return 0;
	uint32 tmpstart = Start;
	uint32 tmpsize = 0;
	while (tmpsize < size){
		struct Frame_Info *ptr_frame_info = 0;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, (void*)tmpstart, PERM_PRESENT | PERM_WRITEABLE);
		Arr[(tmpstart - KERNEL_HEAP_START) / PAGE_SIZE].StartAddress = Start;
		Arr[(tmpstart - KERNEL_HEAP_START) / PAGE_SIZE].Size = size;
		MapPhysicalToVirtual[kheap_physical_address(tmpstart) / PAGE_SIZE] = tmpstart;
		tmpstart += PAGE_SIZE;
		tmpsize += PAGE_SIZE;
	}
	KernalHeapFirstFreeVirtualAddress = tmpstart;
	return (void*)Start;
}
void kfree(void* virtual_address){
	uint32 Start = (uint32)virtual_address;
	Start -= Start % PAGE_SIZE;
	virtual_address = (void*)Start;
	if (Start < KERNEL_HEAP_START || Start >= KERNEL_HEAP_MAX)return;
	while (Start < KERNEL_HEAP_MAX){
		if (Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size){
			if (Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].StartAddress == (uint32)virtual_address){
				MapPhysicalToVirtual[kheap_physical_address(Start) / PAGE_SIZE] = 0;
				unmap_frame(ptr_page_directory, (void*)Start);
				Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].StartAddress = Start;
				Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size = 0;
				Start += PAGE_SIZE;
			}else break;
		}else break;
	}
}
unsigned int kheap_virtual_address(unsigned int physical_address){
	return MapPhysicalToVirtual[physical_address / PAGE_SIZE];
}
unsigned int kheap_physical_address(unsigned int virtual_address){
	uint32 *ptr_page_table = NULL;
	get_page_table(ptr_page_directory, (void*)virtual_address, &ptr_page_table);
	if (ptr_page_table)return (ptr_page_table[PTX(virtual_address)] >> 12) * PAGE_SIZE;
	return 0;
}
void *ContinuousFit(uint32 size){
	if (size > KERNEL_HEAP_MAX - KernalHeapFirstFreeVirtualAddress)return 0;
	return (void*)KernalHeapFirstFreeVirtualAddress;
}
void *FirstFit(uint32 size){
	uint32 Start = KERNEL_HEAP_START;
	uint32 FreeSize = 0;
	while (Start < KERNEL_HEAP_MAX){
		if (!Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size)FreeSize += PAGE_SIZE;
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
	uint32 Start = KernalHeapFirstFreeVirtualAddress;
	uint32 FreeSize = 0;
	uint32 End = KernalHeapFirstFreeVirtualAddress;
	while (Start < KERNEL_HEAP_MAX){
		if (!Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size)FreeSize += PAGE_SIZE;
		else{
			FreeSize = 0;
			if (End == KernalHeapFirstFreeVirtualAddress)End = Start;
		}
		Start += PAGE_SIZE;
		if (FreeSize == size){
			Start -= size;
			break;
		}
	}
	if (FreeSize != size){
		Start = KERNEL_HEAP_START;
		FreeSize = 0;
		while (Start < End){
			if (!Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size)FreeSize += PAGE_SIZE;
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
	uint32 Start = KERNEL_HEAP_START;
	uint32 FreeSize = 0;
	uint32 MinimumSize = KERNEL_HEAP_MAX - KERNEL_HEAP_START + 1;
	while (Start < KERNEL_HEAP_MAX){
		if (!Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size){
			FreeSize += PAGE_SIZE;
			if (Start + PAGE_SIZE == KERNEL_HEAP_MAX && FreeSize >= size && FreeSize < MinimumSize)MinimumSize = FreeSize;
		}else{
			if (FreeSize >= size && FreeSize < MinimumSize)MinimumSize = FreeSize;
			FreeSize = 0;
		}
		Start += PAGE_SIZE;
	}
	if (MinimumSize == KERNEL_HEAP_MAX - KERNEL_HEAP_START + 1)return 0;
	FreeSize = 0;
	Start = KERNEL_HEAP_START;
	while (Start < KERNEL_HEAP_MAX){
		if (!Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size){
			FreeSize += PAGE_SIZE;
			if (Start + PAGE_SIZE == KERNEL_HEAP_MAX && FreeSize == MinimumSize){
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
	uint32 Start = KERNEL_HEAP_START;
	uint32 FreeSize = 0;
	uint32 MaximumSize = 0;
	while (Start < KERNEL_HEAP_MAX){
		if (!Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size){
			FreeSize += PAGE_SIZE;
			if (Start + PAGE_SIZE == KERNEL_HEAP_MAX && FreeSize >= size && FreeSize > MaximumSize)MaximumSize = FreeSize;
		}else{
			if (FreeSize >= size && FreeSize > MaximumSize)MaximumSize = FreeSize;
			FreeSize = 0;
		}
		Start += PAGE_SIZE;
	}
	if (!MaximumSize)return 0;
	FreeSize = 0;
	Start = KERNEL_HEAP_START;
	while (Start < KERNEL_HEAP_MAX){
		if (!Arr[(Start - KERNEL_HEAP_START) / PAGE_SIZE].Size){
			FreeSize += PAGE_SIZE;
			if (Start + PAGE_SIZE == KERNEL_HEAP_MAX && FreeSize == MaximumSize){
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
