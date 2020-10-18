/* ========================================================================
   $File: work/apps/4coder/custom/4coder_casey_index.cpp $
   $Date: 2019/01/29 03:46:25 UTC $
   $Revision: 7 $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright by Molly Rocket, Inc., All Rights Reserved. $
   ======================================================================== */

static Content_Index *global_content_index = 0;

internal String get_name_of_index_type(Indexed_Content_Element_Type content_type)
{
    String result = {};
    
    switch(content_type)
    {
        case ContentType_Unspecified: {result = make_lit_string("Unspecified");} break;
        
        case ContentType_Function: {result = make_lit_string("Function");} break;
        case ContentType_Type: {result = make_lit_string("Type");} break;
        case ContentType_Macro: {result = make_lit_string("Macro");} break;
        case ContentType_EnumValue: {result = make_lit_string("Enum Value");} break;
        case ContentType_ForwardDeclaration: {result = make_lit_string("Forward Declaration");} break;
        
        case ContentType_TODO: {result = make_lit_string("TODO");} break;
        case ContentType_NOTE: {result = make_lit_string("Note");} break;
        case ContentType_IMPORTANT: {result = make_lit_string("Important");} break;
        case ContentType_STUDY: {result = make_lit_string("Study");} break;
    }
    
    return(result);
}

internal tokenizer
Tokenize(String source)
{
    tokenizer Result = {source.str, source.str + source.size, source};
    return(Result);
}

internal tokenizer
Tokenize(tokenizer *parent, Cpp_Token from_token)
{
    tokenizer Result;
    
    Result.source = parent->source;
    Result.At = Result.source.str + from_token.start;
    Result.End = parent->End;
    
    return(Result);
}

internal String
string_from(tokenizer *Tokenizer, Cpp_Token token)
{
    String result = make_string(Tokenizer->source.str + token.start, token.size);
    return(result);
}

inline b32x
IsEndOfLine(char C)
{
    b32x Result = ((C == '\n') ||
                   (C == '\r'));
    
    return(Result);
}

inline b32x
IsWhitespace(char C)
{
    b32x Result = ((C == ' ') ||
                   (C == '\t') ||
                   (C == '\v') ||
                   (C == '\f') ||
                   IsEndOfLine(C));
    
    return(Result);
}

inline b32x
IsAlpha(char C)
{
    b32x Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')));
    
    return(Result);
}

inline b32x
IsNumeric(char C)
{
    b32x Result = ((C >= '0') && (C <= '9'));
    
    return(Result);
}

static void
EatAllWhitespace(tokenizer *Tokenizer)
{
    while(Tokenizer->At != Tokenizer->End)
    {
        if(IsWhitespace(Tokenizer->At[0]))
        {
            ++Tokenizer->At;
        }
        else if(((Tokenizer->At + 1) != Tokenizer->End) &&
                (Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '/'))
        {
            Tokenizer->At += 2;
            while((Tokenizer->At != Tokenizer->End) && !IsEndOfLine(Tokenizer->At[0]))
            {
                ++Tokenizer->At;
            }
        }
        else if(((Tokenizer->At + 1) != Tokenizer->End) &&
                (Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '*'))
        {
            Tokenizer->At += 2;
            while((Tokenizer->At != Tokenizer->End) &&
                  ((Tokenizer->At + 1) != Tokenizer->End) &&
                  !((Tokenizer->At[0] == '*') &&
                    (Tokenizer->At[1] == '/')))
            {
                ++Tokenizer->At;
            }
            
            if((Tokenizer->At != Tokenizer->End) &&
               ((Tokenizer->At + 1) != Tokenizer->End) &&
               (Tokenizer->At[0] == '*') &&
               (Tokenizer->At[1] == '/'))
            {
                Tokenizer->At += 2;
            }
        }
        else
        {
            break;
        }
    }
}

static Cpp_Token
GetToken(tokenizer *Tokenizer)
{
    EatAllWhitespace(Tokenizer);
    
    Cpp_Token Token = {};
    char *Base = Tokenizer->source.str;
    
    if(Tokenizer->At == Tokenizer->End)
    {
        Token.type = CPP_TOKEN_EOF;
    }
    else
    {
        Token.size = 1;
        Token.start = (int32_t)(Tokenizer->At - Base);
        char C = Tokenizer->At[0];
        ++Tokenizer->At;
        switch(C)
        {
            case '(': {Token.type = CPP_TOKEN_PARENTHESE_OPEN;} break;
            case ')': {Token.type = CPP_TOKEN_PARENTHESE_CLOSE;} break;
            case '*': {Token.type = CPP_TOKEN_STAR;} break;
            case '-': {Token.type = CPP_TOKEN_MINUS;} break;
            case '+': {Token.type = CPP_TOKEN_PLUS;} break;
            case '/': {Token.type = CPP_TOKEN_DIV;} break;
            case '%': {Token.type = CPP_TOKEN_MOD;} break;
            case ':': {Token.type = CPP_TOKEN_COLON;} break;
            case ';': {Token.type = CPP_TOKEN_SEMICOLON;} break;
            case ',': {Token.type = CPP_TOKEN_COMMA;} break;
            case '{': {Token.type = CPP_TOKEN_BRACE_OPEN;} break;
            case '}': {Token.type = CPP_TOKEN_BRACE_CLOSE;} break;
            case '=': {Token.type = CPP_TOKEN_EQEQ;} break;
            case '^': {Token.type = CPP_TOKEN_BIT_XOR;} break;
            
            case '"':
            {
                Token.type = CPP_TOKEN_STRING_CONSTANT;
                Token.start = (int32_t)(Tokenizer->At - Base);
                while((Tokenizer->At != Tokenizer->End) &&
                      (Tokenizer->At[0] != '"'))
                {
                    ++Tokenizer->At;
                }
                Token.size = (int32_t)(Tokenizer->At - Base) - Token.start;
                
                if(Tokenizer->At != Tokenizer->End)
                {
                    ++Tokenizer->At;
                }
            } break;
            
            default:
            {
                if(IsNumeric(C))
                {
                    Token.type = CPP_TOKEN_FLOATING_CONSTANT;
                    while((Tokenizer->At != Tokenizer->End) &&
                          (IsNumeric(Tokenizer->At[0]) ||
                           (Tokenizer->At[0] == '.') ||
                           (Tokenizer->At[0] == 'f')))
                    {
                        ++Tokenizer->At;
                        Token.size = (int32_t)(Tokenizer->At - Base) - Token.start;
                    }
                }
                else if(IsAlpha(C) || (C == '_'))
                {
                    Token.type = CPP_TOKEN_IDENTIFIER;
                    while((Tokenizer->At != Tokenizer->End) &&
                          (IsAlpha(Tokenizer->At[0]) ||
                           IsNumeric(Tokenizer->At[0]) ||
                           (Tokenizer->At[0] == '_')))
                    {
                        ++Tokenizer->At;
                        Token.size = (int32_t)(Tokenizer->At - Base) - Token.start;
                    }
                }
                else
                {
                    Token.type = CPP_TOKEN_JUNK;
                }
            } break;
        }
    }
    
    return(Token);
}

static Cpp_Token
PeekToken(tokenizer *Tokenizer)
{
    tokenizer Tokenizer2 = *Tokenizer;
    Cpp_Token Result = GetToken(&Tokenizer2);
    return(Result);
}

static Content_Index *
create_content_index(Application_Links *app)
{
    // TODO(casey): I really just want this to be its own allocating arena, but I can't find anything like that in 4coder?
    // All arenas seem to be fixed size and they can't get more memory if they run out :(
    Content_Index *index = (Content_Index *)malloc(sizeof(Content_Index));
    
    memset(index, 0, sizeof(*index));
    
    return(index);
}

static Indexed_Content_Element **
get_element_slot_for_name(Content_Index *index, String name)
{
    Indexed_Content_Element **slot = &index->name_hash[table_hash_u8((uint8_t *)name.str, name.size) % ArrayCount(index->name_hash)];
    return(slot);
}

static Indexed_Buffer *
get_or_create_indexed_buffer(Content_Index *index, Buffer_ID buffer_id)
{
    Indexed_Buffer *result = 0;
    
    Indexed_Buffer **slot = &index->buffer_hash[buffer_id % ArrayCount(index->buffer_hash)];
    for(Indexed_Buffer *search = *slot;
        search;
        search = search->next_in_hash)
    {
        if(search->buffer_id == buffer_id)
        {
            result = search;
            break;
        }
    }
    
    if(!result)
    {
        result = (Indexed_Buffer *)malloc(sizeof(Indexed_Buffer));
        result->buffer_id = buffer_id;
        result->first_tag = 0;
        result->next_in_hash = *slot;
        *slot = result;
    }
    
    return(result);
}

static color_pair
get_color_pair_for(Indexed_Content_Element_Type type)
{
    color_pair result = {};
    
    switch(type)
    {
        case ContentType_Function:
        {
            result.fore = CC_Function;
        } break;
        
        case ContentType_Type:
        {
            result.fore = CC_Type;
        } break;
        
        case ContentType_Macro:
        {
            result.fore = CC_Macro;
        } break;
        
        case ContentType_EnumValue:
        {
            result.fore = CC_EnumValue;
        } break;
        
        case ContentType_ForwardDeclaration:
        {
            result.fore = CC_ForwardDeclaration;
        } break;
        
        case ContentType_TODO:
        {
            result.fore = CC_TODO;
        } break;
        
        case ContentType_NOTE:
        {
            result.fore = CC_NOTE;
        } break;
        
        case ContentType_IMPORTANT:
        {
            result.fore = CC_IMPORTANT;
        } break;
        
        case ContentType_STUDY:
        {
            result.fore = CC_STUDY;
        } break;
    }
    
    return(result);
}

static Indexed_Content_Element *
begin_element_in_index(Content_Index *index, Indexed_Content_Element_Type type, Buffer_ID buffer_id, int32_t location,
                       int32_t name_size, int32_t content_size)
{
    Indexed_Content_Element *elem = 0;
    
    // TODO(casey): I really just want this to be its own allocating arena, but I can't find anything like that in 4coder?
    // All arenas seem to be fixed size and they can't get more memory if they run out :(
    uint8_t *mem = (uint8_t *)malloc(sizeof(Indexed_Content_Element) + name_size + content_size);
    if(mem)
    {
        elem = (Indexed_Content_Element *)mem;
        
        elem->next_in_hash = 0;
        elem->type = type;
        elem->name.memory_size = elem->name.size = name_size;
        elem->name.str = (char *)(elem + 1);
        elem->content.memory_size = elem->content.size = content_size;
        elem->content.str = elem->name.str + name_size;
        
        elem->color = get_color_pair_for(type);
        
        elem->buffer_id = buffer_id;
        elem->last_known_location = location;
        
        index->total_string_space += name_size + content_size;
        ++index->element_count;
    }
    
    return(elem);
}

static void
end_element_in_index(Content_Index *index, Indexed_Content_Element *elem, u32 flags)
{
    if(elem)
    {
        // NOTE(casey): By convention, we make content into a single line, and remove leading/trailing whitespace
        condense_whitespace(&elem->name);
        condense_whitespace(&elem->content);
        
        Indexed_Content_Element **slot = &index->unhashed;
        if(!(flags & IndexFlag_NoNameLookup))
        {
            slot = get_element_slot_for_name(index, elem->name);
        }
        elem->next_in_hash = *slot;
        *slot = elem;
        
        Indexed_Buffer *buffer = get_or_create_indexed_buffer(index, elem->buffer_id);
        ++buffer->element_type_counts[elem->type];
        ++index->element_type_counts[elem->type];
    }
}

static Indexed_Content_Element *
add_element_to_index(Application_Links *app, Content_Index *index, Indexed_Content_Element_Type type, Buffer_Summary *buffer, int32_t location,
                     int32_t name_start, int32_t name_end,
                     int32_t content_start, int32_t content_end,
                     u32 flags = 0)
{
    Indexed_Content_Element *elem = 0;
    if((name_start <= name_end) &&
       (content_start <= content_end))
    {
        elem = begin_element_in_index(index, type, buffer->buffer_id, location, name_end - name_start, content_end - content_start);
        if(elem)
        {
            buffer_read_range(app, buffer, name_start, name_end, elem->name.str);
            buffer_read_range(app, buffer, content_start, content_end, elem->content.str);
            end_element_in_index(index, elem, flags);
        }
    }
    
    return(elem);
}

static Indexed_Content_Element *
add_element_to_index(Content_Index *index, Indexed_Content_Element_Type type, Buffer_ID buffer_id, int32_t location, String name, String content,
                     u32 flags = 0)
{
    Indexed_Content_Element *elem = begin_element_in_index(index, type, buffer_id, location, name.size, content.size);
    if(elem)
    {
        copy_ss(&elem->name, name);
        copy_ss(&elem->content, content);
        end_element_in_index(index, elem, flags);
    }
    
    return(elem);
}

static Indexed_Content_Tag *
add_tag_to_index(Content_Index *index, Buffer_Summary *buffer, uint32_t type, int32_t start, int32_t end, String content)
{
    Indexed_Content_Tag *tag = 0;
    
    if(start <= end)
    {
        Indexed_Buffer *index_buffer = get_or_create_indexed_buffer(index, buffer->buffer_id);
        // TODO(casey): I really just want this to be its own allocating arena, but I can't find anything like that in 4coder?
        // All arenas seem to be fixed size and they can't get more memory if they run out :(
        uint8_t *mem = (uint8_t *)malloc(sizeof(Indexed_Content_Tag) + content.size);
        if(mem)
        {
            tag = (Indexed_Content_Tag *)mem;
            tag->content.size = tag->content.memory_size = content.size;
            tag->content.str = (char *)(tag + 1);
            copy_ss(&tag->content, content);
            tag->type = type;
            tag->start = start;
            tag->end = end;
            tag->next_in_buffer = index_buffer->first_tag;
            index_buffer->first_tag = tag;
        }
    }
    
    return(tag);
}

static Indexed_Content_Element *
get_element_by_name(Content_Index *index, String name)
{
    Indexed_Content_Element *result = 0;
    
    if(index)
    {
        for(Indexed_Content_Element *elem = *get_element_slot_for_name(index, name);
            elem;
            elem = elem->next_in_hash)
        {
            if(compare_ss(elem->name, name) == 0)
            {
                result = elem;
                break;
            }
        }
    }
    
    return(result);
}

static void
remove_buffer_from_index(Application_Links *app, Content_Index *index, Buffer_ID buffer_id)
{
    for(int hash_index = 0;
        hash_index <= ArrayCount(index->name_hash);
        ++hash_index)
    {
        for(Indexed_Content_Element **elem_ptr = &index->name_hash[hash_index];
            *elem_ptr;
            )
        {
            Indexed_Content_Element *elem = *elem_ptr;
            if(elem->buffer_id == buffer_id)
            {
                *elem_ptr = elem->next_in_hash;
                free(elem);
            }
            else
            {
                elem_ptr = &elem->next_in_hash;
            }
        }
    }
    
    Indexed_Buffer *index_buffer = get_or_create_indexed_buffer(index, buffer_id);
    while(index_buffer->first_tag)
    {
        Indexed_Content_Tag *tag = index_buffer->first_tag;
        index_buffer->first_tag = tag->next_in_buffer;
        free(tag);
    }
    
    for(u32 type_index = 0;
        type_index < ContentType_Count;
        ++type_index)
    {
        index->element_type_counts[type_index] -= index_buffer->element_type_counts[type_index];
        index_buffer->element_type_counts[type_index] = 0;
    }
}

internal calc_mem_slot *
get_or_create_calc_slot(calc_mem *Memory, String name)
{
    calc_mem_slot *result = 0;
    
    for(calc_mem_slot *check = Memory->first_slot;
        check;
        check = check->next)
    {
        if(compare_ss(name, check->name) == 0)
        {
            result = check;
            break;
        }
    }
    
    if(!result)
    {
        result = push_array(Memory->scratch, calc_mem_slot, 1);
        result->next = Memory->first_slot;
        Memory->first_slot = result;
        result->name = name;
        result->value = 0.0f;
    }
    
    return(result);
}

internal double
ExecCalcNode(calc_mem *Memory, calc_node *Node)
{
    double Result = 0.0f;
    
    if(Node)
    {
        double Left = ExecCalcNode(Memory, Node->Left);
        double Right = ExecCalcNode(Memory, Node->Right);
        
        switch(Node->Type)
        {
            case CalcNode_UnaryMinus:
            {
                Result = -Left;
            } break;
            
            case CalcNode_Add:
            {
                Result = Left + Right;
            } break;
            
            case CalcNode_Subtract:
            {
                Result = Left - Right;
            } break;
            
            case CalcNode_Multiply:
            {
                Result = Left * Right;
            } break;
            
            case CalcNode_Divide:
            {
                if(Right != 0)
                {
                    Result = Left / Right;
                }
            } break;
            
            case CalcNode_Mod:
            {
                if(Right != 0)
                {
                    Result = fmod(Left, Right);
                }
            } break;
            
            case CalcNode_Power:
            {
                Result = pow(Left, Right);
            } break;
            
            case CalcNode_Constant:
            {
                Result = Node->Value;
            } break;
            
            case CalcNode_Assign:
            {
                if(Node->Left && (Node->Left->Type == CalcNode_Variable))
                {
                    calc_mem_slot *Slot = get_or_create_calc_slot(Memory, Node->Left->Name);
                    Slot->value = Right;
                }
                Result = Right;
            } break;
            
            case CalcNode_Variable:
            {
                calc_mem_slot *Slot = get_or_create_calc_slot(Memory, Node->Name);
                Result = Slot->value;
            } break;
            
            case CalcNode_Function:
            {
                String func = Node->Name;
                if(compare_sc(func, "sqrt") == 0)
                {
                    Result = sqrt(Left);
                }
                else if(compare_sc(func, "cos") == 0)
                {
                    Result = cos(Left);
                }
                else if(compare_sc(func, "sin") == 0)
                {
                    Result = sin(Left);
                }
                else if(compare_sc(func, "tan") == 0)
                {
                    Result = tan(Left);
                }
                else if(compare_sc(func, "acos") == 0)
                {
                    Result = acos(Left);
                }
                else if(compare_sc(func, "asin") == 0)
                {
                    Result = asin(Left);
                }
                else if(compare_sc(func, "atan") == 0)
                {
                    Result = atan(Left);
                }
                else if(compare_sc(func, "atan2") == 0)
                {
                    Result = atan2(Left, Right);
                }
                else if(compare_sc(func, "cosh") == 0)
                {
                    Result = cosh(Left);
                }
                else if(compare_sc(func, "sinh") == 0)
                {
                    Result = sinh(Left);
                }
                else if(compare_sc(func, "tanh") == 0)
                {
                    Result = tanh(Left);
                }
                else if(compare_sc(func, "pow") == 0)
                {
                    Result = pow(Left, Right);
                }
                else if(compare_sc(func, "floor") == 0)
                {
                    Result = floor(Left);
                }
                else if(compare_sc(func, "ceil") == 0)
                {
                    Result = ceil(Left);
                }
                else if(compare_sc(func, "fabs") == 0)
                {
                    Result = fabs(Left);
                }
                else if(compare_sc(func, "log") == 0)
                {
                    Result = log(Left);
                }
                else if(compare_sc(func, "exp") == 0)
                {
                    Result = exp(Left);
                }
                else if(compare_sc(func, "log10") == 0)
                {
                    Result = log10(Left);
                }
            } break;
            
            default: {Assert(!"AHHHHH!");}
        }
    }
    
    return(Result);
}

internal calc_node *
AddNode(tokenizer *Tokenizer, calc_node_type Type, Cpp_Token Source, calc_node *Left = 0, calc_node *Right = 0)
{
    Partition *scratch = &global_part;
    
    calc_node *Node = push_array(scratch, calc_node, 1);
    Node->Type = Type;
    Node->Name = string_from(Tokenizer, Source);
    Node->Value = 0;
    Node->Left = Left;
    Node->Right = Right;
    return(Node);
}

internal calc_node *
ParseNumber(tokenizer *Tokenizer)
{
    Cpp_Token Token = GetToken(Tokenizer);
    calc_node *Result = AddNode(Tokenizer, CalcNode_Constant, Token);
    
    char Temp[64];
    int Len = sizeof(Temp) - 1;
    if(Len > Token.size)
    {
        Len = Token.size;
    }
    memcpy(Temp, Tokenizer->source.str + Token.start, Len);
    Temp[Len] = 0;
    Result->Value = atof(Temp);
    
    return(Result);
}

internal calc_node *ParseEqualsExpression(tokenizer *Tokenizer);
internal calc_node *ParseAddExpression(tokenizer *Tokenizer);

internal calc_node *
ParseTerm(tokenizer *Tokenizer)
{
    calc_node *Result = 0;
    
    Cpp_Token Token = PeekToken(Tokenizer);
    if(Token.type == CPP_TOKEN_PARENTHESE_OPEN)
    {
        GetToken(Tokenizer);
        Result = ParseEqualsExpression(Tokenizer);
        if(PeekToken(Tokenizer).type == CPP_TOKEN_PARENTHESE_CLOSE)
        {
            GetToken(Tokenizer);
        }
    }
    else if(Token.type == CPP_TOKEN_IDENTIFIER)
    {
        Token = GetToken(Tokenizer);
        if(PeekToken(Tokenizer).type == CPP_TOKEN_PARENTHESE_OPEN)
        {
            GetToken(Tokenizer);
            calc_node *Left = ParseAddExpression(Tokenizer);
            calc_node *Right = 0;
            if(PeekToken(Tokenizer).type == CPP_TOKEN_COMMA)
            {
                Right = ParseAddExpression(Tokenizer);
            }
            if(PeekToken(Tokenizer).type == CPP_TOKEN_PARENTHESE_CLOSE)
            {
                GetToken(Tokenizer);
            }
            Result = AddNode(Tokenizer, CalcNode_Function, Token, Left, Right);
        }
        else
        {
            Result = AddNode(Tokenizer, CalcNode_Variable, Token);
        }
    }
    else if(Token.type == CPP_TOKEN_FLOATING_CONSTANT)
    {
        Result = ParseNumber(Tokenizer);
    }
    
    return(Result);
}

internal calc_node *
ParseUnary(tokenizer *Tokenizer)
{
    calc_node *Result = 0;
    
    Cpp_Token Token = PeekToken(Tokenizer);
    if(Token.type == CPP_TOKEN_MINUS)
    {
        Token = GetToken(Tokenizer);
        Result = AddNode(Tokenizer, CalcNode_UnaryMinus, Token);
        Result->Left = ParseTerm(Tokenizer);
    }
    else
    {
        Result = ParseTerm(Tokenizer);
    }
    
    return(Result);
}

internal calc_node *
ParsePowerExpression(tokenizer *Tokenizer)
{
    calc_node *Result = ParseUnary(Tokenizer);
    Cpp_Token SubToken = PeekToken(Tokenizer);
    if(SubToken.type == CPP_TOKEN_BIT_XOR)
    {
        GetToken(Tokenizer);
        Result = AddNode(Tokenizer, CalcNode_Divide, SubToken, Result, ParsePowerExpression(Tokenizer));
    }
    
    return(Result);
}

internal calc_node *
ParseMultiplyExpression(tokenizer *Tokenizer)
{
    calc_node *Result = ParsePowerExpression(Tokenizer);
    Cpp_Token SubToken = PeekToken(Tokenizer);
    if(SubToken.type == CPP_TOKEN_DIV)
    {
        GetToken(Tokenizer);
        Result = AddNode(Tokenizer, CalcNode_Divide, SubToken, Result, ParseMultiplyExpression(Tokenizer));
    }
    else if(SubToken.type == CPP_TOKEN_STAR)
    {
        GetToken(Tokenizer);
        Result = AddNode(Tokenizer, CalcNode_Multiply, SubToken, Result, ParseMultiplyExpression(Tokenizer));
    }
    
    return(Result);
}

internal calc_node *
ParseAddExpression(tokenizer *Tokenizer)
{
    calc_node *Result = ParseMultiplyExpression(Tokenizer);
    Cpp_Token SubToken = PeekToken(Tokenizer);
    if(SubToken.type == CPP_TOKEN_PLUS)
    {
        GetToken(Tokenizer);
        Result = AddNode(Tokenizer, CalcNode_Add, SubToken, Result, ParseAddExpression(Tokenizer));
    }
    else if(SubToken.type == CPP_TOKEN_MINUS)
    {
        GetToken(Tokenizer);
        Result = AddNode(Tokenizer, CalcNode_Subtract, SubToken, Result, ParseAddExpression(Tokenizer));
    }
    
    return(Result);
}

internal calc_node *
ParseEqualsExpression(tokenizer *Tokenizer)
{
    calc_node *Result = ParseAddExpression(Tokenizer);
    Cpp_Token SubToken = PeekToken(Tokenizer);
    if(SubToken.type == CPP_TOKEN_EQEQ)
    {
        GetToken(Tokenizer);
        Result = AddNode(Tokenizer, CalcNode_Assign, SubToken, Result, ParseEqualsExpression(Tokenizer));
    }
    
    return(Result);
}

static bool32
parse_comment_math(calc_mem *Memory, Application_Links *app, Content_Index *index, Buffer_Summary *buffer, int32_t buffer_offset, tokenizer *Tokenizer)
{
    bool32 result = false;
    
    for(;;)
    {
        calc_node *Node = ParseEqualsExpression(Tokenizer);
        Cpp_Token Token = GetToken(Tokenizer);
        if(Token.type == CPP_TOKEN_SEMICOLON)
        {
            char WholeStr[128];
            char FracStr[128];
            char Combined[512];
            
            // TODO(casey): Allen, if this were to be internationalized, we would need a way to get the decimal/period type for
            // the region.
            char Separator = ',';
            char Decimal = '.';
            char *At = Combined;
            
            double A = ExecCalcNode(Memory, Node);
            if(A < 0)
            {
                A = -A;
                *At = '-';
            }
            
            double Whole = trunc(A);
            double Fractional = A - Whole;
            
            int WholeLen = sprintf(WholeStr, "%.0f", Whole);
            int FracLen = sprintf(FracStr, "%.16f", Fractional);
            
            int SepPos = (WholeLen % 3);
            for(char *Source = WholeStr;
                *Source;
                ++Source, --SepPos)
            {
                if(SepPos == 0)
                {
                    *At++ = Separator;
                    SepPos = 3;
                }
                *At++ = *Source;
            }
            
            if((Whole != A) && (FracLen > 2))
            {
                *At++ = Decimal;
                for(char *Source = FracStr + 2;
                    *Source;
                    ++Source)
                {
                    *At++ = *Source;
                }
            }
            *At = 0;
            
            String label = make_string_slowly(Combined);
            Indexed_Content_Tag *tag = add_tag_to_index(index, buffer, TagType_Label, buffer_offset + Token.start, buffer_offset + Token.start + Token.size, label);
            tag->color.fore = CC_CommentMathResult;
            result = true;
        }
        else
        {
            break;
        }
    }
    
    return(result);
}

static void
parse_comment(Application_Links *app, Content_Index *index, Buffer_Summary *buffer, Cpp_Token comment_token)
{
    Partition *scratch = &global_part;
    Temp_Memory temp_mem = begin_temp_memory(scratch);
    
    calc_mem Memory = {};
    Memory.scratch = scratch;
    
    // TODO(casey): Better constants here (just have hex doubles?)
    double constant_tau = 6.283185307179586476925286766559;
    double constant_e = 2.718281828459045235360287471352;
    get_or_create_calc_slot(&Memory, make_lit_string("tau"))->value = constant_tau;
    get_or_create_calc_slot(&Memory, make_lit_string("pi"))->value = 0.5*constant_tau;
    get_or_create_calc_slot(&Memory, make_lit_string("e"))->value = constant_e;
    
    // NOTE(casey): Get the whole comment
    String comment_contents = scratch_read(app, buffer, comment_token, scratch);
    
    // NOTE(casey): Trim the comment so we're only looking at the content
    int32_t buffer_offset = comment_token.start;
    if(comment_contents.size >= 2)
    {
        if(comment_contents.str[1] == '*')
        {
            // NOTE(casey): It's a C comment
            if(comment_contents.size >= 4)
            {
                buffer_offset += 2;
                comment_contents.str += 2;
                comment_contents.size -= 4;
            }
        }
        else
        {
            // NOTE(casey): It's a CPP comment
            buffer_offset += 2;
            comment_contents.str += 2;
            comment_contents.size -= 2;
        }
    }
    
    // NOTE(casey): Parse the content
    tokenizer Tokenizer = Tokenize(comment_contents);
    Cpp_Token prev_token = PeekToken(&Tokenizer);
    for(;;)
    {
        Cpp_Token token = GetToken(&Tokenizer);
        if(token.type == CPP_TOKEN_EOF)
        {
            break;
        }
        else
        {
            switch(token.type)
            {
                case CPP_TOKEN_IDENTIFIER:
                {
                    Indexed_Content_Element_Type type = ContentType_Unspecified;
                    
                    String value = string_from(&Tokenizer, token);
                    if(compare_sc(value, "NOTE") == 0)
                    {
                        type = ContentType_NOTE;
                    }
                    else if(compare_sc(value, "TODO") == 0)
                    {
                        type = ContentType_TODO;
                    }
                    else if(compare_sc(value, "IMPORTANT") == 0)
                    {
                        type = ContentType_IMPORTANT;
                    }
                    else if(compare_sc(value, "STUDY") == 0)
                    {
                        type = ContentType_STUDY;
                    }
                    
                    if(type != ContentType_Unspecified)
                    {
                        Indexed_Content_Element *indexed = add_element_to_index(
                            app, index, type, buffer, token.start,
                            buffer_offset + token.start,
                            buffer_offset + token.start + token.size,
                            comment_token.start, comment_token.start + Min(64, comment_token.size),
                            IndexFlag_NoNameLookup);
                        Indexed_Content_Tag *tag = add_tag_to_index(index, buffer, TagType_Highlight,
                                                                    buffer_offset + token.start,
                                                                    buffer_offset + token.start + token.size,
                                                                    null_string);
                        tag->color = indexed->color;
                    }
                } break;
                
                case CPP_TOKEN_PARENTHESE_OPEN:
                case CPP_TOKEN_MINUS:
                {
                    // NOTE(casey): These might require a previous token, but also might not.  Try both.
                    tokenizer SubTokenizer = Tokenize(&Tokenizer, prev_token);
                    if(parse_comment_math(&Memory, app, index, buffer, buffer_offset, &SubTokenizer))
                    {
                        Tokenizer = SubTokenizer;
                    }
                    else
                    {
                        SubTokenizer = Tokenize(&Tokenizer, token);
                        if(parse_comment_math(&Memory, app, index, buffer, buffer_offset, &SubTokenizer))
                        {
                            Tokenizer = SubTokenizer;
                        }
                    }
                } break;
                
                case CPP_TOKEN_STAR:
                case CPP_TOKEN_PLUS:
                case CPP_TOKEN_DIV:
                case CPP_TOKEN_MOD:
                case CPP_TOKEN_EQEQ:
                case CPP_TOKEN_BIT_XOR:
                {
                    // NOTE(casey): These require a previous token, so only try parsing from there.
                    tokenizer SubTokenizer = Tokenize(&Tokenizer, prev_token);
                    if(parse_comment_math(&Memory, app, index, buffer, buffer_offset, &SubTokenizer))
                    {
                        Tokenizer = SubTokenizer;
                    }
                } break;
            }
            prev_token = token;
        }
    }
    
    end_temp_memory(temp_mem);
}

static void
add_buffer_to_index(Application_Links *app, Content_Index *index, Buffer_Summary *buffer)
{
    ++index->buffer_count;
    
    Token_Iterator iter = iterate_tokens(app, buffer, 0);
    
    int32_t paren_level = 0;
    int32_t brace_level = 0;
    
    while(iter.valid)
    {
        Cpp_Token token = get_next_token(app, &iter);
        
        if(token.flags & CPP_TFLAG_PP_BODY)
        {
            // TODO(casey): Do we need to pick up macros here?
        }
        else
        {
            switch(token.type)
            {
                case CPP_TOKEN_COMMENT:
                {
                    // NOTE(casey): Comments can contain special operations that we want to perform, so we thunk to a special parser to handle those
                    parse_comment(app, index, buffer, token);
                } break;
                
                case CPP_PP_DEFINE:
                {
                    int32_t content_start = token.start;
                    token = get_next_token(app, &iter);
                    
                    // TODO(casey): Allen, how do I scan for the next "not continued newline"?
                    int32_t content_end = token.start + token.size;
                    
                    add_element_to_index(app, index, ContentType_Macro, buffer, token.start,
                                         token.start, token.start + token.size,
                                         content_start, content_end);
                } break;
                
                case CPP_TOKEN_KEY_TYPE_DECLARATION:
                {
                    bool32 forward_declaration = true;
                    int32_t content_start = token.start;
                    Cpp_Token element_name = {};
                    Cpp_Token last_identifier = {};
                    
                    String typedef_keyword = make_lit_string("typedef");
                    if(token_text_is(app, buffer, token, typedef_keyword))
                    {
                        // NOTE(casey): Typedefs can't be "forward declared", so we always record their location.
                        forward_declaration = false;
                        
                        // NOTE(casey): This is a simple "usually works" parser for typedefs.  It is well-known that C grammar
                        // doesn't actually allow you to parse typedefs properly without actually knowing the declared types
                        // at the time, which of course we cannot do since we parse per-file, and don't even know things we
                        // would need (like -D switches, etc.)
                        
                        // TODO(casey): If eventually this were upgraded to a more thoughtful parser, struct/union/enum defs
                        // would be parsed from brace to brace, so they could be handled recursively inside of a typedef for
                        // the pattern "typedef struct {} foo;", which currently we do not handle (we ignore the foo).
                        int typedef_paren_level = 0;
                        while(iter.valid)
                        {
                            token = get_next_token(app, &iter);
                            if(token.type == CPP_TOKEN_IDENTIFIER)
                            {
                                last_identifier = token;
                            }
                            else if(token.type == CPP_TOKEN_PARENTHESE_OPEN)
                            {
                                if(typedef_paren_level == 0)
                                {
                                    // NOTE(casey): If we are going back into a parenthetical, it means that whatever the last
                                    // identifier we saw is our best candidate.
                                    element_name = last_identifier;
                                }
                                ++typedef_paren_level;
                            }
                            else if(token.type == CPP_TOKEN_PARENTHESE_CLOSE)
                            {
                                --typedef_paren_level;
                            }
                            else if(token.type == CPP_TOKEN_KEY_TYPE_DECLARATION)
                            {
                                // TODO(casey): If we _really_ wanted to, we could parse the type here recursively,
                                // and then we could capture things that were named differently a la "typedef struct foo {...} bar;"
                                // but I think that should wait until the 4coder parser is more structural, because I don't
                                // know how much parsing we _really_ want to do here.
                                forward_declaration = true;
                                break;
                            }
                            else if(token.type == CPP_TOKEN_SEMICOLON)
                            {
                                break;
                            }
                        }
                        
                        if(element_name.size == 0)
                        {
                            element_name = last_identifier;
                        }
                    }
                    
                    if(!element_name.size)
                    {
                        while(iter.valid)
                        {
                            token = peek_token(app, &iter);
                            if(token.type == CPP_TOKEN_IDENTIFIER)
                            {
                                element_name = token;
                                get_next_token(app, &iter);
                            }
                            else if((token.type == CPP_TOKEN_KEY_MODIFIER) ||
                                    (token.type == CPP_TOKEN_KEY_QUALIFIER) ||
                                    (token.type == CPP_TOKEN_KEY_ACCESS) ||
                                    (token.type == CPP_TOKEN_KEY_LINKAGE))
                            {
                                // NOTE(casey): Let it go.
                                get_next_token(app, &iter);
                            }
                            else if(token.type == CPP_TOKEN_BRACE_OPEN)
                            {
                                // NOTE(casey): It's probably type definition
                                forward_declaration = false;
                                break;
                            }
                            else
                            {
                                // NOTE(casey): It's probably a forward declaration
                                break;
                            }
                        }
                    }
                    
                    int32_t content_end = token.start;
                    
                    if(element_name.size)
                    {
                        Indexed_Content_Element_Type type = ContentType_Type;
                        if(forward_declaration)
                        {
                            type = ContentType_ForwardDeclaration;
                        }
                        
                        add_element_to_index(app, index, type, buffer, element_name.start,
                                             element_name.start, element_name.start + element_name.size,
                                             content_start, content_end);
                    }
                } break;
                
                case CPP_TOKEN_PARENTHESE_OPEN:
                {
                    ++paren_level;
                } break;
                
                case CPP_TOKEN_PARENTHESE_CLOSE:
                {
                    --paren_level;
                } break;
                
                case CPP_TOKEN_BRACE_OPEN:
                {
                    if((paren_level == 0) &&
                       (brace_level == 0))
                    {
                        // NOTE(casey): This is presumably a function, see if we can find it's name.
                        int32_t content_end = token.start;
                        int32_t content_start = content_end;
                        int32_t name_start = 0;
                        int32_t name_end = 0;
                        
                        Token_Iterator back = iter;
                        get_prev_token(app, &back);
                        get_prev_token(app, &back);
                        Cpp_Token comment_pass = get_prev_token(app, &back);
                        while(comment_pass.type == CPP_TOKEN_COMMENT)
                        {
                            comment_pass = get_prev_token(app, &back);
                        }
                        if(comment_pass.type == CPP_TOKEN_PARENTHESE_CLOSE)
                        {
                            paren_level = -1;
                            while(back.valid)
                            {
                                if(token.type != CPP_TOKEN_COMMENT)
                                {
                                    content_start = token.start;
                                }
                                token = get_prev_token(app, &back);
                                if((paren_level == 0) &&
                                   (brace_level == 0) &&
                                   ((token.flags & CPP_TFLAG_PP_BODY) ||
                                    (token.type == CPP_TOKEN_BRACE_CLOSE) ||
                                    (token.type == CPP_TOKEN_SEMICOLON) ||
                                    (token.type == CPP_TOKEN_EOF)))
                                {
                                    break;
                                }
                                else
                                {
                                    switch(token.type)
                                    {
                                        case CPP_TOKEN_PARENTHESE_OPEN:
                                        {
                                            ++paren_level;
                                        } break;
                                        
                                        case CPP_TOKEN_PARENTHESE_CLOSE:
                                        {
                                            --paren_level;
                                        } break;
                                        
                                        case CPP_TOKEN_BRACE_OPEN:
                                        {
                                            ++brace_level;
                                        } break;
                                        
                                        case CPP_TOKEN_BRACE_CLOSE:
                                        {
                                            --brace_level;
                                        } break;
                                        
                                        case CPP_TOKEN_IDENTIFIER:
                                        {
                                            if((paren_level == 0) &&
                                               (brace_level == 0) &&
                                               (name_start == 0))
                                            {
                                                name_start = token.start;
                                                name_end = token.start + token.size;
                                                Token_Iterator probe = back;
                                                get_next_token(app, &probe);
                                                for(;;)
                                                {
                                                    Cpp_Token test = get_next_token(app, &probe);
                                                    if((test.type == 0) ||
                                                       (test.type == CPP_TOKEN_EOF) ||
                                                       (test.type == CPP_TOKEN_PARENTHESE_CLOSE) ||
                                                       (test.type == CPP_TOKEN_PARENTHESE_OPEN))
                                                    {
                                                        name_end = test.start;
                                                        break;
                                                    }
                                                }
                                            }
                                        } break;
                                    }
                                }
                            }
                            
                            if(name_start < name_end)
                            {
                                Indexed_Content_Element_Type type = ContentType_Function;
                                add_element_to_index(app, index, type, buffer, name_start,
                                                     name_start, name_end,
                                                     content_start, content_end);
                            }
                            
                            brace_level = 0;
                            paren_level = 0;
                        }
                    }
                    
                    ++brace_level;
                } break;
                
                case CPP_TOKEN_BRACE_CLOSE:
                {
                    --brace_level;
                } break;
            }
        }
    }
}

static void
add_all_buffers_to_index(Application_Links *app, Content_Index *index)
{
    for(Buffer_Summary buffer = get_buffer_first(app, AccessAll);
        buffer.exists;
        get_buffer_next(app, &buffer, AccessAll))
    {
        int32_t Unimportant = true;
        buffer_get_setting(app, &buffer, BufferSetting_Unimportant, &Unimportant);
        
        if(buffer.tokens_are_ready && !Unimportant)
        {
            add_buffer_to_index(app, index, &buffer);
        }
    }
}

static void
update_index_for_buffer(Application_Links *app, Content_Index *index, Buffer_Summary *buffer)
{
    remove_buffer_from_index(app, index, buffer->buffer_id);
    add_buffer_to_index(app, index, buffer);
}

static Content_Index *
get_global_content_index(Application_Links *app)
{
    Content_Index *result = global_content_index;
    if(!result)
    {
        global_content_index = create_content_index(app);
        add_all_buffers_to_index(app, global_content_index);
        result = global_content_index;
    }
    
    return(result);
}

static void
jump_to_element(Application_Links *app, View_Summary *view, Indexed_Content_Element *elem)
{
    if (view->buffer_id != elem->buffer_id)
    {
        view_set_buffer(app, view, elem->buffer_id, 0);
    }
    Buffer_Seek seek;
    seek.type = buffer_seek_pos;
    seek.pos = elem->last_known_location;;
    view_set_cursor(app, view, seek, true);
}

static void
jump_to_element_activate(Application_Links *app, Partition *scratch, Heap *heap,
                         View_Summary *view, Lister_State *state,
                         String text_field, void *user_data, bool32 activated_by_mouse)
{
    lister_default(app, scratch, heap, view, state, ListerActivation_Finished);
    if(user_data)
    {
        jump_to_element(app, view, (Indexed_Content_Element *)user_data);
    }
}

typedef bool32 element_type_predicate(Application_Links *app, Content_Index *index, Indexed_Content_Element *elem, void *user_data);

static bool32
element_is_definition(Application_Links *app, Content_Index *index, Indexed_Content_Element *elem, void *user_data)
{
    bool32 result = ((elem->type == ContentType_Function) ||
                     (elem->type == ContentType_Type) ||
                     (elem->type == ContentType_Macro) ||
                     (elem->type == ContentType_EnumValue));
    return(result);
}

static bool32
element_type_equals(Application_Links *app, Content_Index *index, Indexed_Content_Element *elem, void *user_data)
{
    bool32 result = (elem->type == (uint64_t)user_data);
    return(result);
}

static void
jump_to_element_lister(Application_Links *app, char *Label, element_type_predicate *predicate, void *user_data = 0)
{
    Partition *arena = &global_part;
    
    View_Summary view = get_active_view(app, AccessAll);
    view_end_ui_mode(app, &view);
    Temp_Memory temp = begin_temp_memory(arena);
    
    Content_Index *index = get_global_content_index(app);
    
    Lister_Option *options = push_array(arena, Lister_Option, index->element_count);
    int32_t option_index = 0;
    for(int hash_index = 0;
        hash_index <= ArrayCount(index->name_hash);
        ++hash_index)
    {
        for(Indexed_Content_Element *elem = index->name_hash[hash_index];
            elem;
            elem = elem->next_in_hash)
        {
            if(predicate(app, index, elem, user_data))
            {
                options[option_index].text_color = get_color(elem->color.fore);
                options[option_index].pop_color = get_color(CC_DefaultText);
                options[option_index].back_color = get_color(elem->color.back);
                options[option_index].string = elem->name;
                options[option_index].status = elem->content;
                options[option_index].user_data = elem;
                ++option_index;
            }
        }
    }
    begin_integrated_lister__basic_list(app, Label, jump_to_element_activate, 0, 0, options, option_index, index->total_string_space, &view);
    end_temp_memory(temp);
}
