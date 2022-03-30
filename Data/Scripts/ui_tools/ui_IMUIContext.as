class IMUIContextWrapper {
    int imui_context_id = -1;

    IMUIContextWrapper() {
        imui_context_id = CreateIMUIContext();
        Get().Init();
    }

    ~IMUIContextWrapper() {
        DisposeIMUIContext(imui_context_id);
    }

    IMUIContext@ Get() {
        IMUIContext@ ctx = GetIMUIContext(imui_context_id);
        if( ctx is null ) {
            imui_context_id = CreateIMUIContext();
            Get().Init();
            return Get();
        } else {
            return ctx;
        }
    }
}
