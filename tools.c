#include "tools.h"
#include <vdr/sources.h>



/** ********** CompareBySourceName(const void *a, const void *b) ***************
***
*** assume params are pointers to int variable that represent sources
*** String compare source 'Description' / Name
*******************************************************************************/
int CompareBySourceName(const void *a, const void *b)
{
    int src_a = *(int *)a;
    int src_b = *(int *)b;

    const char* src_a_name = "";
    const char* src_b_name = "";

    if (Sources.Get(src_a))
        src_a_name = Sources.Get(src_a)->Description();
    if (Sources.Get(src_b))
        src_b_name = Sources.Get(src_b)->Description();

    return strcmp(src_a_name, src_b_name);
}



/** *********************** cMySourceList **************************************
*** cMySourceList should hold list of sources, represented by numbers
*** (see cSource in vdr ), without any duplicates
*******************************************************************************/

cMySourceList::~cMySourceList ()
{
    Clear();
}

int cMySourceList::Find(int code) const
{
    for (int i=0; i< Size(); ++i)
    {
        if (At(i) == code)
            return true;
    } // for

    return -1; // not found
}

void cMySourceList::Clear()
{
    cVector<int>::Clear();
}


/** const char* BouquetName(int chanNumber) ************************************
*** Get bouquet name of a channel
********************************************************************************/

const char* ChannelInBouquet(const cChannel *channel, cChannels& Channels)
{
    // if channel is bouquet name then it does not belong to any bouquet
    // as it is not a "channel"
    if (!channel || channel->GroupSep()) return NULL;

#if 0 // very slow for large channel lists
    // for a channel list with 3632 lines with 76 bouquets,
    // a bouquet search with partialmatch=true took ~12secs, where as the
    // alternative is near instantaneous
    int grpIdx = Channels.GetPrevGroup(channel->Index());
    cChannel *groupCh = Channels.Get(grpIdx);
    return groupCh ? groupCh->Name() : NULL;
#else
    while (channel && !channel->GroupSep()) channel = Channels.Prev(channel);
    return channel? channel->Name():NULL;
#endif
}


const cEvent* GetPresentEvent(const cChannel* channel)
{
    const cEvent* event = NULL;
    if (channel)
    {
        cSchedulesLock schedulesLock;
        const cSchedules *schedules = cSchedules::Schedules(schedulesLock);

        if (schedules)
        {
            const cSchedule* schedule = schedules->GetSchedule(channel->GetChannelID());

            if (schedule)
                event = schedule->GetPresentEvent();

        } // if

    } // if (bouquet)

    return event;
}


char NumpadToChar(unsigned int k, unsigned int multiples)
{
    static char numpad[][5] = {
        " @+", //0
        ".?!", // 1
        "abc", //2
        "def", //3
        "ghi", //4
        "jkl", //5
        "mno", //6
        "pqrs", //7
        "tuv", //8
        "wxyz", //9
    };

    char ret = 0; // error, not found

    if (k>9) return ret; // error, index over shoot

    // maximum multiple for given number 'k', multiples start from 0...
    int maxMultiple = strlen(numpad[k])-1;

#if 0
    if (multiples > maxMultiple) return ret; // error, too many multiples
#else
    multiples %= strlen(numpad[k]);
#endif

    ret = numpad[k][multiples];
    return ret;

}

char NumpadChar(eKeys Key)
{
#define NUMPAD_TIMEOUT 2000 //2 secs
            static cTimeMs timeout(NUMPAD_TIMEOUT);
            static int multiples = 0;
            static int lastNum = -1;

            int pressedNum = Key - k0;

            if (timeout.TimedOut() || lastNum != pressedNum) {
                multiples = 0;
                lastNum = pressedNum;
            }
            else
                ++multiples;

            timeout.Set(NUMPAD_TIMEOUT);

            return NumpadToChar(pressedNum, multiples);
}

#ifndef REELVDR
// create a new string on the heap that has a maximum 'maxlen' number of
// characters (each can span multiple bytes)
//
// tries to find end of words to break the string
// returned char* (if not NULL) must be freed by caller
//
// bytesCopied used by the caller to jump in the input string
char* BreakString(const char*str, int maxlen, int &bytesCopied)
{
	bytesCopied = 0;
	if (!str) return NULL;

	// number of chars
	int symCount = Utf8StrLen(str);

	// convert UTF-8 multi-byte char to a single uint number
	uint *inp_sym = new uint[symCount+1]; // +1 for the '\0'
	int inp_sym_len = Utf8ToArray(str, inp_sym, symCount+1);

	// number of chars in the string less than the maxlen
	// TODO: if there is a '\n' in this part!
	if (inp_sym_len <= maxlen)
	{
		// copy the input string
		bytesCopied = strlen(str);

		delete[] inp_sym;
		//printf("** just copied **\n");
		return strdup(str);
	}

	// create string on heap that can hold maxlen (multi-byte)chars
	char *s = (char*) malloc((4*maxlen+1)*sizeof(char)); //+1 for '\0'

	int i = 0;

	// if no wordbreaks found, break after maxlen characters
	int line_end = maxlen;

	// donot show the char that was used to break the string
	// for eg. ' ' is unnecessary at the end/beginning of a line
	int skip_symbol = 0;

	//the string here is longer than requested. Break it down.
	for (i = 0; i < maxlen; ++i)
	{
		//look for word breaks and new lines
		if (inp_sym[i] == '\0' || inp_sym[i] == '\n')
		{
			line_end = i;
			if (inp_sym[i] != '\0')
				skip_symbol = 1;
			break; // found end of line exit loop
		}
		else if (inp_sym[i] == ' ') //space
		{
			line_end = i;
			skip_symbol = 1;
		}
	}//for

	// create char from uint array, since the char* is large enough
	// 	it will be '\0' terminated
	bytesCopied = Utf8FromArray(inp_sym, s, 4*maxlen-1,line_end);

	if (skip_symbol)// jump over the last symbol since it is a ' ' or '\n'
		++bytesCopied;

	delete[] inp_sym;
	return s;
}
// breaks given text into smaller 'maxChar' length strings and
// displays them as unselectable text on OSD
void cUtils::AddFloatingText(cOsdMenu *m, const char* text, int maxChars)
{
        // empty string; display nothing
        if(!text) return;

        char *p = NULL;
        int copied = 0;

        //printf("\tFloating Text (%i): '%s'\n", maxChars, text);
        while(*text)
        {
                // returns a pointer to a char string with atmost maxChars characters (not bytes)
                // p has to be freed; p is NULL if text was null
                p = BreakString(text, maxChars, copied);
                text += copied; //jump bytes not chars
                if(p)
                {
                        m->Add(new cOsdItem(p, osUnknown,false)); // unselectable text
                        free(p); p = NULL;
                }
        }
}
#endif /* REELVDR */

