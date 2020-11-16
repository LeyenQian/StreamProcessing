
#ifndef __LIST_MANAGE_H
#define __LIST_MANAGE_H

#include <Windows.h>

/*================================================================================
*
* Function:		InitListHead
*
* Description:	initial list header
*
* Arguments:	PLIST_ENTRY	[IN]	pHeadListEntry
*
* Return:		VOID
*
================================================================================*/
VOID InitListHead( PLIST_ENTRY pHeadListEntry )
{
	pHeadListEntry -> Flink = pHeadListEntry;
	pHeadListEntry -> Blink = pHeadListEntry;

	return;
}

/*================================================================================
*
* Function:		IsListEmpty
*
* Description:	determin the list is empty or not
*
* Arguments:	PLIST_ENTRY	[IN]	pHeadListEntry
*
* Return:		BOOL
*
================================================================================*/
BOOL IsListEmpty( PLIST_ENTRY pHeadListEntry )
{
	if ( pHeadListEntry ->Flink == pHeadListEntry && pHeadListEntry ->Blink == pHeadListEntry )
	{
		return TRUE;
	}

	return FALSE;
}

/*================================================================================
*
* Function:		ListHeadInsert
*
* Description:	insert node at front
*
* Arguments:	PLIST_ENTRY	[IN]	pHeadListEntry
*				PLIST_ENTRY	[IN]	pListEntry
*
* Return:		BOOL
*
================================================================================*/
VOID ListHeadInsert(PLIST_ENTRY pHeadListEntry, PLIST_ENTRY pListEntry)
{
	pListEntry -> Flink = pHeadListEntry -> Flink;
	pListEntry -> Blink = pHeadListEntry;  

	pHeadListEntry -> Flink -> Blink = pListEntry;
	pHeadListEntry -> Flink = pListEntry;

	return;
}

/*================================================================================
*
* Function:		ListTailInsert
*
* Description:	inset node at end
*
* Arguments:	PLIST_ENTRY	[IN]	pHeadListEntry
*				PLIST_ENTRY	[IN]	pListEntry
*
* Return:		BOOL
*
================================================================================*/
VOID ListTailInsert(PLIST_ENTRY pHeadListEntry, PLIST_ENTRY pListEntry)
{
	pListEntry -> Flink = pHeadListEntry;
	pListEntry -> Blink = pHeadListEntry -> Blink;  

	pHeadListEntry -> Blink -> Flink = pListEntry;
	pHeadListEntry -> Blink  = pListEntry;

	return;
}

/*================================================================================
*
* Function:	ListInsert
*
* Description:	insert node at given position
*
* Arguments:	PLIST_ENTRY	[IN]	pBlinkEntry
*				PLIST_ENTRY	[IN]	pFlinkEntry
*				PLIST_ENTRY	[IN]	pListEntry
*
* Return:	BOOL
*
================================================================================*/
VOID ListInsert( PLIST_ENTRY pBlinkEntry, PLIST_ENTRY pFlinkEntry, PLIST_ENTRY pListEntry )
{
	pListEntry -> Blink = pBlinkEntry;
	pListEntry -> Flink = pFlinkEntry;

	pBlinkEntry -> Flink = pListEntry;
	pFlinkEntry -> Blink = pListEntry;

	return;
}

/*================================================================================
*
* Function:		ListEntryDelete
*
* Description:	delete node
*
* Arguments:	PLIST_ENTRY	[IN]	pListEntry
*
* Return:		BOOL
*
================================================================================*/
VOID ListEntryDelete( PLIST_ENTRY pListEntry )
{
	pListEntry -> Blink -> Flink = pListEntry -> Flink;
	pListEntry -> Flink -> Blink = pListEntry -> Blink;

	return;	
}

#endif