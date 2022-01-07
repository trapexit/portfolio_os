/* File : AccessText.c */

#include "access.h"
#include "doaccess.h"
#include "AccessText.h"
#include "AccessUtility.h"


#define	IsBreakChar(a) (((a) == ' ') || ((a) == '-') || ((a) == '|'))




void
Text_GetManyStrings(strstruct, instring, maxstrlen, maxstrings)
ManyStrings	*strstruct;
char		*instring;
int32		maxstrlen, maxstrings;
{
	int32	strbreakpoint;
	int32	strcnt;
	int32	maxlen;

	strcnt = 0;
	maxlen = 0;

	while (*instring && (strcnt < maxstrings)) {
		if ((strbreakpoint = ((int32) strlen(instring))) > maxstrlen) {
			strbreakpoint = maxstrlen - 1;
			if (!Text_FindGoodBreakInText(instring, &strbreakpoint)) {
				strbreakpoint = maxstrlen - 1;
			}
			strbreakpoint++;
		}

		strstruct->ms_Strings[strcnt] = (char *) malloc((strbreakpoint + 1) * sizeof(char));
		strncpy(strstruct->ms_Strings[strcnt], instring, strbreakpoint);
		strstruct->ms_Strings[strcnt][strbreakpoint] = '\0';
		instring += strbreakpoint;

		if (strstruct->ms_Strings[strcnt][strbreakpoint - 1] == ' ') {
			strbreakpoint--;
			strstruct->ms_Strings[strcnt][strbreakpoint] = '\0';
		}

		if (strbreakpoint > maxlen) maxlen = strbreakpoint;
		strcnt++;
	}

	strstruct->ms_Count = strcnt;
	strstruct->ms_MaxLength = maxlen;
}



void
Text_DisposeManyStrings(strstruct)
ManyStrings	*strstruct;
{
	int32	count;

	count = strstruct->ms_Count;

	while(count) {
		count--;
		free(strstruct->ms_Strings[count]);
	}
}



int32
Text_FindGoodBreakInText(text, position)
char	*text;
int32	*position;
{
	int32	found = 0;

	while (*position && !found) {
		if (IsBreakChar(text[*position])) found++;
		else (*position)--;
	}

	return(found);
}



void
Text_StringCreate(otext, itext)
char	**otext;
char	*itext;
{
	*otext = (char *) malloc((strlen(itext) + 1) * sizeof(char));
	strcpy(*otext, itext);
}



// Function: FntGetTextWidth
// Return:	 Given fontinfo and character, returns the pixel width of the character
// Modifies: None
// Requires: string to be null terminated
int32 FntGetTextWidth(
char *text)
{
  int32 i, countwidth;

  i = countwidth = 0;

  while (text[i] != '\0'){
    countwidth += FntGetCharWidth(text[i], TRUE);
	i++;
  }

  return countwidth;
}



// Function: FntGetCharWidth
// Return:	 Given fontinfo and character, returns the pixel width of the character
// Modifies: None
// Requires:
int32 FntGetCharWidth(
char ch,
Boolean DefaultMaxWid)
{
  Font *CurrentFont;
  FontEntry *fe;

  // еее potential slowdown have to call this for every character
  CurrentFont = GetCurrentFont();
  fe = CurrentFont->font_FontEntries;

  while((fe != NULL) && (fe->ft_CharValue != '\0')){
    if (fe->ft_CharValue == (int32)ch){
	  return (fe->ft_Width);
	}

	if (((int32)ch) < fe->ft_CharValue){
	  fe = fe->ft_LesserBranch;
	}
	else{
	  fe = fe->ft_GreaterBranch;
	}
  }

  if (DefaultMaxWid)
  	//еее returns the max width if ch is not in font set
  	return FntGetMaxCharWidth();
  else
    return ERR_NO_CHR_IN_FONT;
}



// Function: FntGetCurrentFontHeight
// Return:	 returns the pixel height of the font
// Modifies: None
// Requires:
int32 FntGetCurrentFontHeight()
{
  Font *CurrentFont;

  CurrentFont = GetCurrentFont();
  return (int32)CurrentFont->font_Height;
}



// Function: FntGetVisCharLenInRect
// Return:	 Number of characters rect given the text.
//			 Right now it always starts from the first character in line.
// Modifies: None
// Requires:
int32 FntGetVisCharLenInRect(
Rect *rect,
char *text)
{
  int32 dispwidth, countwidth, width;
  int32 i;

  countwidth = width = 0;

  dispwidth = rect->rect_XRight - rect->rect_XLeft;
  for (i = 0; (i < MAX_FILENAME_LEN) && (text[i] != '\0'); i++){
  	countwidth += width;
  	width = FntGetCharWidth(text[i], TRUE);
	if (countwidth + width > dispwidth)
	  return i;
  }

  return (i);
}



// Function: FntDrawTextInRect
// Return:	 error code
// Modifies: None
// Requires: text is null terminated
int32 FntDrawTextInRect(
ScreenDescr	*screen,
GrafCon	*localGrafCon,
Rect	*rect,
char	*text)
{
	int32	textlen;
	char	drawtext[MAX_FILENAME_LEN + 3];

	// Calculate the number of char that can fit into the display
  	textlen = FntGetVisCharLenInRect(rect, text);

	if (textlen < strlen(text))
		{
	    strncpy(drawtext, text, textlen);
		strcpy(&drawtext[textlen-3], "...");
		}
	else
		{
	    strncpy(drawtext, text, strlen(text) + 1);
		}

    localGrafCon->gc_PenX = rect->rect_XLeft;
    localGrafCon->gc_PenY = rect->rect_YTop;
	DrawWooText(localGrafCon, screen->sd_BitmapItem, drawtext);

	return SUCCESS;
}



int32 FntGetMaxCharWidth()
{
  Font *CurrentFont;
  FontEntry *fe;

  // еее potential slowdown have to call this for every character
  CurrentFont = GetCurrentFont();
  fe = CurrentFont->font_FontEntries;

  return _fntGetMaxCharWidth(fe);
}



int32 _fntGetMaxCharWidth(
FontEntry *fe)
{
  int32 maxwidth, tempwidth;

  if (fe->ft_CharValue == '\0') return 0;

  maxwidth = fe->ft_Width;
  maxwidth = (maxwidth < (tempwidth = _fntGetMaxCharWidth(fe->ft_LesserBranch)) )? tempwidth: maxwidth;
  maxwidth = (maxwidth < (tempwidth = _fntGetMaxCharWidth(fe->ft_GreaterBranch)) )? tempwidth: maxwidth;

  return maxwidth;
}


