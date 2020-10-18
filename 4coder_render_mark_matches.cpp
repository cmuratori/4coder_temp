internal void
casey_render_mark_matches(Application_Links *app, View_Summary *view, Range on_screen_range, Buffer_Summary *buffer, Managed_Scope render_scope,
                          Partition *scratch)
{
    Content_Index *index = get_global_content_index(app);
    Indexed_Buffer *index_buffer = get_or_create_indexed_buffer(index, buffer->buffer_id);
    View_ID view_id = view->view_id;
    Face_ID font_id = get_default_font_for_view(app, view->view_id);
    
    Query_Bar *active_query_bar = 0;
    get_active_query_bars(app, view_id, 1, &active_query_bar);
    
    int_color query_color = get_color(CC_SearchHighlight);
    
    Temp_Memory temp = begin_temp_memory(scratch);
    int32_t text_size = on_screen_range.one_past_last - on_screen_range.first;
    char *text = push_array(scratch, char, text_size);
    buffer_read_range(app, buffer, on_screen_range.first, on_screen_range.one_past_last, text);
    
    for(Indexed_Content_Tag *tag = index_buffer->first_tag;
        tag;
        tag = tag->next_in_buffer)
    {
        if(interval_overlap(tag->start, tag->end, on_screen_range.first, on_screen_range.one_past_last))
        {
            if(tag->type == TagType_Label)
            {
                // Vec2 BeginP = screen_p_from_abs(app, view, tag->start);
                // Vec2 EndP = screen_p_from_abs(app, view, tag->end);
                Full_Cursor end;
                if(cursor_from_abs(app, view, tag->end, &end))
                {
                    float y = (view->unwrapped_lines) ? end.unwrapped_y : end.wrapped_y;
                    Buffer_Seek seek = seek_xy(max_f32, y, 1, view->unwrapped_lines);
                    Full_Cursor eol;
                    if(view_compute_cursor(app, view, seek, &eol))
                    {
                        Vec2 draw_p = screen_p_from_abs(app, view, eol.pos);
                        draw_string(app, font_id, tag->content, draw_p.x + view->line_height, draw_p.y, get_color(tag->color.fore), 0, 1.0f, 0.0f);
                    }
                }
            }
            else if(tag->type == TagType_Highlight)
            {
                casey_render_highlight_region(app, buffer->buffer_id, tag->start, tag->end, get_color(tag->color.fore), get_color(tag->color.back), render_scope);
            }
        }
    }
    
    Token_Iterator iter = iterate_tokens(app, buffer, on_screen_range.first);
    while(iter.valid)
    {
        Cpp_Token token = get_next_token(app, &iter);
        if(token.start < on_screen_range.one_past_last)
        {
            if(token.type == CPP_TOKEN_IDENTIFIER)
            {
                String name = scratch_read(app, buffer, token, scratch);
                // TODO(casey): Merge types together here and pick the color?
                int_color fore = 0;
                int_color back = 0;
                
                Indexed_Content_Element *elem = get_element_by_name(index, name);
                while(elem &&
                      elem->next_in_hash &&
                      (elem->type == ContentType_ForwardDeclaration))
                {
                    Indexed_Content_Element *next = elem->next_in_hash;
                    if(compare_ss(next->name, name) == 0)
                    {
                        elem = next;
                    }
                    else
                    {
                        break;
                    }
                }
                
                if(elem)
                {
                    fore = get_color(elem->color.fore);
                    back = get_color(elem->color.back);
                }
                
                if(fore || back)
                {
                    casey_render_highlight_region(app, buffer->buffer_id, token.start, token.start + token.size, fore, back, render_scope);
                }
            }
        }
    }
    
    Highlight_Record *records = push_array(scratch, Highlight_Record, 0);
    String tail = make_string(text, text_size);
    for (int32_t i = 0; i < text_size; tail.str += 1, tail.size -= 1, i += 1){
        int_color ForeColor = 0;
        int32_t absolute_position = i + on_screen_range.first;
        // int_color BackColor = 0;
        String v = {};
        
        if(active_query_bar)
        {
            String query_term = active_query_bar->string;
            // TODO(casey): Ideally we would have more information from the query system about what
            // to highlight.  This is really just a stop-gap for highlighting which assumes a priori
            // a case-insensitive search, but that doesn't work for general queries, so really _they_
            // need to be able to be asked here what should be highlighted (or have done this search
            // themselves into a temporary buffer we can render from).
            if (match_part_insensitive(tail, query_term)){
                ForeColor = query_color;
                v = query_term;
            }
        }
        
        if(v.size)
        {
            Highlight_Record *record = push_array(scratch, Highlight_Record, 1);
            record->first = absolute_position;
            record->one_past_last = record->first + v.size;
            record->color = ForeColor;
            // TODO(casey): I'd like to be able to set a background color here, but that doesn't seem possible?
            // record->back_color = BackColor;
            tail.str += v.size - 1;
            tail.size -= v.size - 1;
            i += v.size - 1;
        }
    }
    
    // TODO(casey): Allen, is this actually safe?  It assumes the allocator can never allocate
    // discontinuous memory, which seems like a big ask, although maybe the idea here is just that
    // you just keep committing forward in the virtual address space?
    int32_t record_count = (int32_t)(push_array(scratch, Highlight_Record, 0) - records);
    push_array(scratch, Highlight_Record, 1);
    
    // NOTE(casey): The end_temp_memory() in here looks a little suspicious to me, it's just the way Allen's
    // code was doing it, so I left it that way...
    if (record_count > 0){
        sort_highlight_record(records, 0, record_count);
        Temp_Memory marker_temp = begin_temp_memory(scratch);
        Marker *markers = push_array(scratch, Marker, 0);
        int_color current_color = records[0].color;
        {
            Marker *marker = push_array(scratch, Marker, 2);
            marker[0].pos = records[0].first;
            marker[1].pos = records[0].one_past_last;
        }
        for (int32_t i = 1; i <= record_count; i += 1){
            bool32 do_emit = i == record_count || (records[i].color != current_color);
            if (do_emit){
                int32_t marker_count = (int32_t)(push_array(scratch, Marker, 0) - markers);
                Managed_Object o = alloc_buffer_markers_on_buffer(app, buffer->buffer_id, marker_count, &render_scope);
                managed_object_store_data(app, o, 0, marker_count, markers);
                Marker_Visual v = create_marker_visual(app, o);
                marker_visual_set_effect(app, v,
                                         VisualType_CharacterHighlightRanges,
                                         SymbolicColor_Transparent, current_color, 0);
                marker_visual_set_priority(app, v, VisualPriority_Lowest);
                end_temp_memory(marker_temp);
                current_color = records[i].color;
            }
            
            Marker *marker = push_array(scratch, Marker, 2);
            marker[0].pos = records[i].first;
            marker[1].pos = records[i].one_past_last;
        }
    }
    
    end_temp_memory(temp);
}