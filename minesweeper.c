#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include <termios.h>    // termios, TCSANOW, ECHO, ICANON
#include <unistd.h>     // STDIN_FILENO

#define MAP(X,Y) *(map.mapPtr + (X)*map.y + (Y))
#define SLOT(X,Y) *(map.slotsPtr + (X)*map.y + (Y))

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

enum slot { hidden, marked, discovered };

enum userAction { mark, discover, unmark, discoverNearby, skip };

enum interactiveKeyAction { up, down, left, right, toggleMark, discoverMaybeNearby, wrong };

struct mapStructure {
    int x;
    int y;
    int mines;
    short int * mapPtr;
    enum slot * slotsPtr;
};

struct userInputStructure {
    int x;
    int y;
    enum userAction action; 
};

struct mapStructure map;

struct userInputStructure userInput;

bool continueGame = true;

int minesLeft;

unsigned int startTime;


void display_debug (void) {
    int iX, iY;
    
    printf (GRN);
    printf ("***-----DEBUG INFORMATION-----***\n");
    printf ("map.x = %i\n", map.x);
    printf ("map.y = %i\n", map.y);
    printf ("map.mines = %i\n", map.mines);
    printf ("map.mapPtr :\n");
    for ( iY = map.y - 1; iY >= 0; iY--)
    {
        for ( iX = 0; iX < map.x; iX++)
        {
            printf ("% i ", MAP(iX, iY));
        }
        printf ("\n");
    }
    printf ("map.slotsPtr :\n");
    
    
    for ( iY = map.y - 1; iY >= 0; iY--)
    {
        for ( iX = 0; iX < map.x; iX++)
        {
            printf ("% i ", SLOT(iX, iY));
        }
        printf ("\n");
    }
    printf (RESET);
}


void get_custom_map_config (void) {
    int result;
    bool reenterValue;

    map.y = 0;
    
    printf ("\nEnter the size of the map:\n");
    
    do
    {
        reenterValue = false;
        map.x = 0;
        printf ("X: ");
        result = scanf("%i", &(map.x));
        if ( result != 1 || !(map.x >= 1 && map.x < 75) )
        {
            scanf ("%*[^\n]%*c");
            printf (RED "\nPlease enter a number between 1 and 75.\n\n" RESET);
            reenterValue = true;
        }
        
    } while ( reenterValue );
    
    do
    {
        reenterValue = false;
        map.y = 0;
        printf ("Y: ");
        result = scanf("%i", &(map.y));
        if ( result != 1 || !(map.y >= 1 && map.y < 75) )
        {
            scanf ("%*[^\n]%*c");
            printf (RED "\nPlease enter a number between 1 and 75.\n\n" RESET);
            reenterValue = true;
        }
        
    } while ( reenterValue );
    
    do
    {
        reenterValue = false;
        map.mines = 0;
        printf ("Mines: ");
        result = scanf("%i", &(map.mines));
        if ( result != 1 || !(map.mines >= 1) || (map.mines > (map.x * map.y)) )
        {
            scanf ("%*[^\n]%*c");
            printf (RED "\nPlease enter a number between 1 and %i.\n\n" RESET, (map.x * map.y));
            reenterValue = true;
        }
        
    } while ( reenterValue );

}


void get_map_config (void) {
    void get_custom_map_config (void);

    int result, level;
    bool reenterValue;
    
    printf ("Levels:\n");
    printf (GRN "[1]: Beginner (9x9 with 10 mines)\n" RESET);
    printf (YEL "[2]: Intermediate (16x16 with 40 mines)\n" RESET);
    printf (RED "[3]: Expert (16x30 with 99 mines)\n" RESET);
    printf (CYN "[4]: Custom\n" RESET);
    
    do
    {
        reenterValue = false;
        level = 0;
        printf ("\nEnter the level number: ");
        result = scanf("%i", &level);
        if ( result != 1 || !(level >= 1 && level < 5)  )
        {
            scanf ("%*[^\n]%*c");
            printf (RED "\nPlease enter a number from 1 to 4.\n" RESET);
            reenterValue = true;
        }
        
    } while ( reenterValue );

    switch (level)
    {
    case 1:
        map.x = 9;
        map.y = 9;
        map.mines = 10;
        break;
    case 2:
        map.x = 16;
        map.y = 16;
        map.mines = 40;
        break;
    case 3:
        map.x = 40;
        map.y = 16;
        map.mines = 99;
        break;
    case 4:
        get_custom_map_config ();
        break;
    default:
        printf (RED "\nIt should'nt be possible to be here...\n" RESET);
        exit(EXIT_FAILURE);
        break;
    }
}


int compare_function(const void *a,const void *b) {
    int *x = (int *) a;
    int *y = (int *) b;
    return *x - *y;
}


int rand_lim(int limit) {
    int divisor = RAND_MAX/(limit+1);
    int retval;
    static bool timeIsNotInit = true;

    if ( timeIsNotInit )
    {
        timeIsNotInit = false;
        time_t current_time;
        srand ( (unsigned) time(&current_time) );    
    }

    do { 
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}


void fill_mines (void) {
    int rand_lim(int limit);
    int compare_function(const void *a, const void *b);
    
    int minesPlaces[map.mines];
    int i, j;
    bool isSame;

    for ( i = 0; i < map.mines; i++)
    {
        do
        {
            minesPlaces[i] = rand_lim ( (map.x * map.y) - 1 );
            isSame = false;
            for ( j = 0; j < i; j++)
            {
                if ( minesPlaces[j] == minesPlaces[i] )
                {
                    isSame = true; 
                    break;   
                }
            }
        } while ( isSame );        
    }
    
    qsort (minesPlaces, map.mines, sizeof(*minesPlaces), compare_function);
    
    for ( i = 0, j = 0; j < map.mines; i++)
    {
        if ( i == minesPlaces[j] )
        {
            *(map.mapPtr + i) = -1;
            ++j;
        }
    }
}


int valide_coord (int coord_X, int coord_Y) {
    if ( (coord_X >= 0 && coord_X < map.x) && (coord_Y >= 0 && coord_Y < map.y) )
    {
        return 1;
    }

    return 0;
}


void fill_numbers (void) {
    int iX, iY;
    int tX, tY;
    int cX, cY;
    int number;
    
    for ( iX = 0; iX < map.x; iX++)
    {
        for ( iY = 0; iY < map.y; iY++)
        {
            if ( MAP(iX, iY) == -1 )
            {
                continue;
            }
            
            number = 0;

            for ( tX = -1; tX < 2; tX++)
            {
                for ( tY = -1; tY < 2; tY++)
                {
                    cX = iX + (tX);
                    cY = iY + (tY);
                    if ( valide_coord(cX, cY) )
                    {
                        if ( MAP(cX, cY) == -1 )
                        {
                            number++;
                        } 
                    }
                }
            }

            MAP(iX, iY) = number;
        }
    }
}


void generate_map (void) {
    void fill_mines (void);
    void fill_number (void);

    short int *mapPtr =  (short int *) malloc (map.x * map.y * sizeof(short int));
    if ( mapPtr == NULL )
    {
        printf ("\nAn error occured while allocating memory.\n");
        exit(EXIT_FAILURE);
    }

    map.mapPtr = mapPtr;

    for (int i = 0; i < map.x*map.y; i++)
    {
        *(map.mapPtr + i) = 0;
    }

    fill_mines ();
    fill_numbers ();
}


void generate_slots (void) {
    int i;
    
    enum slot *slotsPtr = (enum slot *) malloc (map.x * map.y * sizeof(enum slot));
    if ( slotsPtr == NULL )
    {
        printf ("\nAn error occured while allocating memory.\n");
        exit(EXIT_FAILURE);    
    }
    map.slotsPtr = slotsPtr;
    
    for ( i = 0; i < (map.x * map.y); i++)
    {
        *(map.slotsPtr + i) = hidden;
    }
}


void showRules (void) {
    printf ("\n");
    printf ("Welcome to Minesweeper!\n");
    printf ("The rules are the same as in the original Minesweeper.\n");
    printf ("Use WASD, ZQSD or the arrows on your keyboard to navigate on the map.\n");
    printf ("Press SPACE to unhide a slot and X to mark one.\n");
    printf ("By pressing SPACE on an alredy discovered slot, you unhide all the surouding slots except the marked ones.\n");
    printf ("Have fun!\n\n");
}


void init_game (void) {
    void showRules (void);
    void get_map_config (void);
    void generate_map (void);
    void generate_slots (void);
    
    showRules ();

    userInput.x = 0;
    userInput.y = 0;

    get_map_config ();
    
    minesLeft = map.mines;

    generate_map ();
    generate_slots ();
}


char getCharDisplayColor (int iX, int iY, char **colorPtr) {
    switch ( SLOT(iX, iY) )
    {
    case hidden:
        *colorPtr = MAG;
        return '?';
        break;

    case marked:
        *colorPtr = RED;
        return '!';
        break;

    default:
        if ( MAP(iX, iY) == -1 )
        {
            *colorPtr = RED;
            return '*';
        } else
        {
            switch (MAP(iX, iY))
            {
            case 0:
                *colorPtr = GRN;
                break;
            case 1:
                *colorPtr = CYN;
                break;
            case 2:
                *colorPtr = BLU;
                break;
            case 3:
                *colorPtr = YEL;
                break;
            default:
                *colorPtr = RED;
                break;
            }
            return ( MAP(iX, iY) + '0' );
        }
        break;
    }
}


void displayLineBreak (int Y) {
    int iX;
    bool rightY = false;

    if ( Y == userInput.y || Y == (userInput.y + 1) )
    {
        rightY = true;
    }

    printf (" ");
    
    if ( rightY && userInput.x == 0 )
    {
        printf (RED);
    }
    printf ("+");

    for ( iX = 0; iX < map.x; iX++)
    {
        if ( rightY && userInput.x == iX )
        {
            printf (RED);
        }

        printf ("---");
        
        if ( rightY && userInput.x == iX + 1 )
        {
            printf (RED);
        }

        printf("+" RESET);
    }
    printf ("\n");
}


void display_map (void) {
    char getCharDisplayColor (int iX, int iY, char **colorPtr);
    void displayLineBreak (int Y);

    int iY, iX;
    char *color;
    char toDisplay;
    bool rightY;
    
    printf ("\x1B[H");
    
    for ( iY = map.y - 1; iY >= 0; iY--)
    {
        if ( iY == userInput.y )
        {
            rightY = true;
        } else
        {
            rightY = false;
        }
        
        displayLineBreak( iY + 1 );
        
        if ( rightY && userInput.x == 0 )
        {
            printf (RED);
        }
        
        printf (" | " RESET);

        for ( iX = 0; iX < map.x; iX++)
        {
            toDisplay = getCharDisplayColor(iX, iY, &color);
            printf ("%s" "%c" RESET, color, toDisplay );

            if ( ((iX == userInput.x -1) || (iX == (userInput.x))) && rightY )
            {
                printf (RED);
            }
            printf (" | " RESET);
            
        }
        printf ("\n");
    }
    displayLineBreak( 0 );
    
    printf ("\n\n");
    if ( continueGame == true )
    {
        printf ("Mines left: \x1B[K%i", minesLeft);
    }
}


enum interactiveKeyAction getInteractiveAction (void) {
    char buffer[3];

    buffer[0] = getchar();

    if ( islower (buffer[0]) )
    {
        buffer[0] = toupper(buffer[0]);
    }
    
    switch (buffer[0])
    {
    case 'Z':
    case 'W':
        return up;
        break;
    case 'Q':
    case 'A':
        return left;
        break;
    case 'S':
        return down;
        break;
    case 'D':
        return right;
        break;
    case ' ':
        return discoverMaybeNearby;
        break;
    case 'X':
        return toggleMark;
        break;
    case ((char) 27):
        if ( (getchar()) == (char) 91 )
        {
            switch ( getchar() )
            {
            case 'A':
                return up;
                break;
            case 'B':
                return down;
                break;
            case 'C':
                return right;
                break;
            case 'D':
                return left;
                break;
            default:
                return wrong;
                break;
            }
        }

        return wrong;
        break;
    default:
        return wrong;
        break;
    }
}


void executeInteractiveKey (enum interactiveKeyAction currentAction) {
    int valide_coord (int coord_X, int coord_Y);

    int cX = userInput.x, cY = userInput.y;

    switch ( currentAction )
    {
    case up:
        cY += 1;
        break;
    case down:
        cY -= 1;
        break;
    case left:
        cX -= 1;
        break;
    case right:
        cX += 1;
        break;
    default:
        printf (RESET "\n\nSTARFOULA dans executeInteractiveKey()\n");
        exit(EXIT_FAILURE);
        break;
    }

    if ( valide_coord(cX, cY) )
    {
        userInput.x = cX;
        userInput.y = cY;
    }
    
}


void getUserInput (void) {
    enum interactiveKeyAction getInteractiveAction (void);
    void executeInteractiveKey (enum interactiveKeyAction currentAction);
    
    enum interactiveKeyAction rawAction;
    bool continueLoop;

    do
    {
        continueLoop = false;
        rawAction = getInteractiveAction();

        switch ( rawAction )
        {
        case up:
        case left:
        case right:
        case down:
            executeInteractiveKey (rawAction);
            userInput.action = skip;
            break;
        case toggleMark:
            if ( SLOT(userInput.x, userInput.y) != discovered )
            {
                if ( SLOT(userInput.x, userInput.y) == marked )
                {
                    userInput.action = unmark;
                } else
                {
                    userInput.action = mark;
                }
            } else
            {
                userInput.action = skip;
            }
            break;
        case discoverMaybeNearby:
            if ( SLOT(userInput.x, userInput.y) == discovered )
            {
                userInput.action = discoverNearby;
            } else
            {
                userInput.action = discover;
            }
            break;
        default:
            continueLoop = true;
            break;
        }

    } while ( continueLoop );
    
    
}


void displayGameTime (void) {
    time_t currentTime;
    unsigned int totalTime = time (&currentTime) - startTime;

    if ( totalTime > 60 )
    {
        if ( totalTime / 60 != 1 )
        {
            printf ("%u minutes and %u second", totalTime / 60, totalTime % 60);
        } else
        {
            printf ("1 minute and %u second", totalTime % 60);
        }

        if ( (totalTime % 60) != 1 )
        {
            printf ("s");
        }
        
    } else
    {
        printf ("%u seconds", totalTime);
    }
}


void endGame (void) {
    void displayGameTime (void);
    
    int i;

    userInput.x = -1;
    userInput.y = -1;

    for ( i = 0; i < (map.x * map.y); i++)
    {
        *(map.slotsPtr + i) = discovered;
    }
    
    printf ("\r");
    printf (RED "!!! BOOM - GAME OVER (and it took you a whole ");
    displayGameTime ();
    printf (" to loose a %ix%i board game with %i mine", map.x, map.y, map.mines);
    
    if ( map.mines > 1 )
    {
        printf ("s");
    }

    printf (") !!!\n" RESET);

    continueGame = false;
}


void unhideNearbyValues (int initialX, int initialY, bool isZero) {
    int valide_coord (int coord_X, int coord_Y);

    int cX, cY;
    int tX, tY;

    SLOT (initialX, initialY) = discovered;

    for ( tX = -1; tX < 2; tX++)
    {
        for ( tY = -1; tY < 2; tY++)
        {
            cX = initialX + (tX);
            cY = initialY + (tY);

            if ( valide_coord (cX, cY) )
            {
                if ( SLOT(cX, cY) == hidden )
                {
                    if ( MAP(cX, cY) == 0 )
                    {
                        unhideNearbyValues (cX, cY, 1);
                    }
                    else
                    {
                        if ( isZero )
                        {
                            SLOT(cX, cY) = discovered;
                        }
                    }
                }
            }

        }
    }
}


void unhideValue (int X, int Y) {
    void unhideNearbyValues (int initialX, int initialY, bool isZero);
    void endGame (void);

    switch ( MAP (X,Y) )
        {
        case -1:
            endGame ();
            break;
        case 0:
            unhideNearbyValues (X, Y, 1);
            break;
        default:
            unhideNearbyValues (X, Y, 0);
            break;
        }
}


void executeAction (void) {
    void unhideValue (int X, int Y);

    int initialX = userInput.x, initialY = userInput.y;
    int cX, cY;
    int tX, tY;

    switch (userInput.action)
    {
    case mark:
        SLOT (initialX, initialY) = marked;
        --minesLeft;
        break;
    case unmark:
        SLOT (initialX, initialY) = hidden;
        ++minesLeft;
        break;
    case discover:
        unhideValue (initialX, initialY);
        break;
    case skip:
        break;
    default:

        for ( tX = -1; tX < 2; tX++)
        {
            for ( tY = -1; tY < 2; tY++)
            {
                cX = initialX + (tX);
                cY = initialY + (tY);
                if ( valide_coord(cX, cY) )
                {
                    if ( SLOT(cX, cY) == hidden )
                    {
                        unhideValue (cX, cY);    
                    }
                }
            }
        }

        break;
    }
}


void checkWin (void) {
    void displayGameTime (void);
    
    int i;
    
    for ( i = 0; i < (map.x * map.y); i++)
    {
        if ( *(map.mapPtr + i) == -1 )
        {
            if ( *(map.slotsPtr + i) == discovered )
            {
                return;
            } else
            {
                continue;
            }
            
        } else if ( *(map.slotsPtr + i) == hidden )
        {
            return;
        }
    }

    if ( minesLeft < 0 )
    {
        return;
    }

    userInput.x = -1;
    userInput.y = -1;

    printf ("\r");
    printf (GRN "!!! Congradulations - You won a %ix%i board game with %i mine", map.x, map.y, map.mines);
    
    if (map.mines > 1)
    {
        printf ("s");
    }
    
    printf(" in ");
    displayGameTime ();
    printf (" !!!\n" RESET);

    for ( i = 0; i < (map.x * map.y); i++)
    {
        *(map.slotsPtr + i) = discovered;
    }

    continueGame = false;    
}


int main (void) {
    void init_game (void);
    void display_map (void);
    void getUserInput (void);
    void executeAction (void);
    void checkWin (void);

    time_t currentTime;
    startTime = time (&currentTime);
    
    static struct termios oldt, newt;

    init_game ();

    tcgetattr ( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
    printf ("\x1B[2J\x1B[H");
    printf("\e[?25l");

    // display_debug ();
    
    display_map ();
    do
    {
        getUserInput ();
        executeAction ();
        checkWin (); 
        display_map ();   
    } while ( continueGame );
    
    printf ("\n\n");
    printf ("Thanks for playing, the game was written by Daniil Korobitsin.\n\n");
    printf("\e[?25h");
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

    return EXIT_SUCCESS;
}
