/*
 * Copyright (c) 2018 https://www.thecoderscorner.com (Nutricherry LTD).
 * This product is licensed under an Apache license, see the LICENSE file in the top-level directory.
 */

/**
 * Adafruit_GFX renderer that renders menus onto this type of display. This file is a plugin file and should not
 * be directly edited, it will be replaced each time the project is built. If you want to edit this file in place,
 * make sure to rename it first.
 * 
 * LIBRARY REQUIREMENT
 * This library requires the AdaGfx library along with a suitable driver.
 */

#include "tcMenuAdaFruitGfx.h"

extern const ConnectorLocalInfo applicationInfo;

int drawingCount = 0;

#if DISPLAY_HAS_MEMBUFFER == true
    #define refreshDisplayIfNeeded(gr, needUpd) {if(needUpd) reinterpret_cast<Adafruit_ILI9341*>(gr)->display();}
#else
    #define refreshDisplayIfNeeded(g, n)
#endif

Coord textExtents(Adafruit_GFX* graphics, const char* text, int16_t x, int16_t y) {
	int16_t x1, y1;
	uint16_t w, h;
	graphics->getTextBounds((char*)text, x, y, &x1, &y1, &w, &h);

    serdebugF4("Textbounds (y1, w, h): ", y1, w, h);
	return Coord(w, h);
}

void AdaFruitGfxMenuRenderer::setGraphicsDevice(Adafruit_GFX* gfx, AdaColorGfxMenuConfig *gfxConfig) {

	if (gfxConfig->editIcon == nullptr || gfxConfig->activeIcon == nullptr) {
		gfxConfig->editIcon = defEditingIcon;
		gfxConfig->activeIcon = defActiveIcon;
		gfxConfig->editIconWidth = 16;
		gfxConfig->editIconHeight = 12;
	}

	graphics = gfx;

	auto& factory = propertiesFactory;
	// TEXT, BACKGROUND, HIGHLIGHT1, HIGHLIGHT2, SELECTED_FG, SELECTED_BG
	color_t paletteItems[] { gfxConfig->fgItemColor, gfxConfig->bgItemColor, gfxConfig->bgSelectColor, gfxConfig->fgSelectColor};
	color_t titleItems[] { gfxConfig->fgTitleColor, gfxConfig->bgTitleColor, gfxConfig->fgTitleColor, gfxConfig->fgSelectColor};

	int titleHeight = heightForFontPadding(gfxConfig->titleFont, gfxConfig->titleFontMagnification, gfxConfig->titlePadding);
	int itemHeight = heightForFontPadding(gfxConfig->itemFont, gfxConfig->itemFontMagnification, gfxConfig->itemPadding);
	factory.setDrawingPropertiesDefault(ItemDisplayProperties::COMPTYPE_ACTION, paletteItems, gfxConfig->itemPadding, gfxConfig->itemFont, gfxConfig->itemFontMagnification, 0, itemHeight, GridPosition::JUSTIFY_LEFT_NO_VALUE);
	factory.setDrawingPropertiesDefault(ItemDisplayProperties::COMPTYPE_ITEM, paletteItems, gfxConfig->itemPadding, gfxConfig->itemFont, gfxConfig->itemFontMagnification, 0, itemHeight, GridPosition::JUSTIFY_TITLE_LEFT_VALUE_RIGHT);
	factory.setDrawingPropertiesDefault(ItemDisplayProperties::COMPTYPE_TITLE, titleItems, gfxConfig->titlePadding, gfxConfig->itemFont, gfxConfig->itemFontMagnification, gfxConfig->titleBottomMargin, titleHeight, GridPosition::JUSTIFY_LEFT_NO_VALUE);
    factory.setSelectedColors(gfxConfig->bgSelectColor, gfxConfig->fgSelectColor);

	factory.addImageToCache(DrawableIcon(SPECIAL_ID_EDIT_ICON, Coord(gfxConfig->editIconWidth, gfxConfig->editIconHeight), DrawableIcon::ICON_XBITMAP, gfxConfig->editIcon));
	factory.addImageToCache(DrawableIcon(SPECIAL_ID_ACTIVE_ICON, Coord(gfxConfig->editIconWidth, gfxConfig->editIconHeight), DrawableIcon::ICON_XBITMAP, gfxConfig->activeIcon));

    setDisplayDimensions(graphics->width(), graphics->height());
}

int AdaFruitGfxMenuRenderer::heightForFontPadding(const GFXfont* font, int mag, const MenuPadding& padding) {
    graphics->setFont((const GFXfont*)font);
    graphics->setTextSize(mag);
    return textExtents(graphics, "(Ag", 30,30).y + padding.top + padding.bottom;
}

void AdaFruitGfxMenuRenderer::drawWidget(Coord where, TitleWidget *widget, color_t colFg, color_t colBg) {
    serdebugF3("Drawing widget pos,icon: ", where.x, widget->getCurrentState());
    redrawNeeded = true;

    graphics->fillRect(where.x, where.y, widget->getWidth(), widget->getHeight(), colBg);

    graphics->drawXBitmap(where.x, where.y, widget->getCurrentIcon(), widget->getWidth(), widget->getHeight(), colFg);
}

void AdaFruitGfxMenuRenderer::drawMenuItem(GridPositionRowCacheEntry* entry, Coord where, Coord areaSize) {
    redrawNeeded = true;
    switch(entry->getPosition().getDrawingMode()) {
        case GridPosition::DRAW_TEXTUAL_ITEM:
        case GridPosition::DRAW_INTEGER_AS_UP_DOWN:
        case GridPosition::DRAW_TITLE_ITEM:
            drawTextualItem(entry, where, areaSize);
            break;
        case GridPosition::DRAW_INTEGER_AS_SCROLL:
            if(entry->getMenuItem()->getMenuType() != MENUTYPE_INT_VALUE) return; // disallowed
            drawSlider(entry, reinterpret_cast<AnalogMenuItem*>(entry->getMenuItem()), where, areaSize);
            break;
        case GridPosition::DRAW_AS_ICON_ONLY:
        case GridPosition::DRAW_AS_ICON_TEXT:
            //drawIconItem(theItem, where, areaSize, mode == GridPosition::DRAW_AS_ICON_TEXT);
            break;
    }
}

void AdaFruitGfxMenuRenderer::drawingCommand(BaseGraphicalRenderer::RenderDrawingCommand command) {
    switch(command) {
        case DRAW_COMMAND_CLEAR: {
            auto* pCfg = propertiesFactory.configFor(nullptr, ItemDisplayProperties::COMPTYPE_ITEM);
            graphics->fillScreen(pCfg->getColor(ItemDisplayProperties::BACKGROUND));
            serdebugF("cls");
            redrawNeeded = true;
            break;
        }
        case DRAW_COMMAND_START:
            break;
        case DRAW_COMMAND_ENDED:
            if(redrawNeeded) {
                refreshDisplayIfNeeded(graphics, requiresUpdate);
                redrawNeeded = false;
            }
            break;
    }
}

int AdaFruitGfxMenuRenderer::drawCoreLineItem(GridPositionRowCacheEntry* entry, DrawableIcon* icon, const Coord &where, const Coord &size) {
    auto pad = entry->getDisplayProperties()->getPadding();
    serdebugF4("Drawing at: ", where.y, size.x, size.y);

    entry->getMenuItem()->setChanged(false); // we are drawing the item so it's no longer changed.

    auto bgColor = entry->getDisplayProperties()->getColor(ItemDisplayProperties::BACKGROUND);
    auto textColor = entry->getDisplayProperties()->getColor(ItemDisplayProperties::TEXT);

    int imgMiddleY = where.y + ((size.y - icon->getDimensions().y) / 2);
    if(entry->getMenuItem()->isEditing() || entry->getMenuItem()->isActive()) {
        graphics->fillRect(where.x, where.y, size.x, size.y, bgColor);
        graphics->drawXBitmap(pad.left, imgMiddleY, icon->getIcon(false), icon->getDimensions().x, icon->getDimensions().y, textColor);
        graphics->setTextColor(textColor);
    }
    else {
        graphics->fillRect(where.x, where.y, size.x, size.y, bgColor);
        graphics->setTextColor(textColor);
    }
    return imgMiddleY;
}

void AdaFruitGfxMenuRenderer::internalDrawText(GridPositionRowCacheEntry* pEntry, Coord where, Coord size) {
    GridPosition::GridJustification just = pEntry->getPosition().getJustification();
    auto padding = pEntry->getDisplayProperties()->getPadding();

    const auto *font = static_cast<const GFXfont *>(pEntry->getDisplayProperties()->getFont());
    graphics->setFont(font);
    if(pEntry->getMenuItem()->isActive() || pEntry->getMenuItem()->isEditing()) {
        graphics->setTextColor(propertiesFactory.getSelectedColor(ItemDisplayProperties::TEXT));
    }
    else {
        graphics->setTextColor(pEntry->getDisplayProperties()->getColor(ItemDisplayProperties::TEXT));
    }

    if(just == GridPosition::JUSTIFY_TITLE_LEFT_VALUE_RIGHT) {
        // special case, title left, value right.
        int drawingPositionY = where.y + (size.y - padding.bottom);
        graphics->setCursor(where.x + padding.left, drawingPositionY);
        pEntry->getMenuItem()->copyNameToBuffer(buffer, bufferSize);
        serdebugF4("item: ", buffer, size.y, where.y);
        graphics->print(buffer);

        copyMenuItemValue(pEntry->getMenuItem(), buffer, bufferSize);
        int16_t right = size.x - (textExtents(graphics, buffer, 0, 30).x + padding.right);
        graphics->setCursor(right, drawingPositionY);
        graphics->print(buffer);
    }
    else {
        char sz[32];
        if(itemNeedsValue(just))
            copyMenuItemNameAndValue(pEntry->getMenuItem(), sz, sizeof sz, ':');
        else
            pEntry->getMenuItem()->copyNameToBuffer(sz, sizeof sz);

        int startPosition = 0;
        if(just == GridPosition::JUSTIFY_RIGHT_WITH_VALUE || just == GridPosition::JUSTIFY_RIGHT_NO_VALUE) {
            startPosition = size.x - (textExtents(graphics, sz, 0, 30).x + padding.right);
        }
        else if(just == GridPosition::JUSTIFY_CENTER_WITH_VALUE || just == GridPosition::JUSTIFY_CENTER_NO_VALUE) {
            startPosition = ((size.x - textExtents(graphics, sz, 0, 30).x) / 2) + padding.right;
        }
        int yCursor = font ? ((where.y + size.y) - padding.bottom) : (where.y + padding.top);
        graphics->setCursor(startPosition + where.x, yCursor);
        graphics->print(sz);
        serdebugF4("intTx ", sz, startPosition + where.x, (where.y + size.y) - padding.bottom);
    }
}

void AdaFruitGfxMenuRenderer::drawTextualItem(GridPositionRowCacheEntry* pEntry, Coord where, Coord size) {
    auto* icon = propertiesFactory.iconForMenuItem(pEntry->getMenuItem()->isEditing() ? SPECIAL_ID_EDIT_ICON :SPECIAL_ID_ACTIVE_ICON);
    drawCoreLineItem(pEntry, icon, where, size);
    auto padding = pEntry->getDisplayProperties()->getPadding();

    auto* editIcon = propertiesFactory.iconForMenuItem(SPECIAL_ID_ACTIVE_ICON);
    int iconOffset = (editIcon) ? editIcon->getDimensions().x + padding.left : 0;

    internalDrawText(pEntry, where, size);
}

void AdaFruitGfxMenuRenderer::drawSlider(GridPositionRowCacheEntry* entry, AnalogMenuItem* pItem, Coord where, Coord size) {
    auto* icon = propertiesFactory.iconForMenuItem(entry->getMenuItem()->isEditing() ? SPECIAL_ID_EDIT_ICON :SPECIAL_ID_ACTIVE_ICON);
    drawCoreLineItem(entry, icon, where, size);
    MenuPadding pad = entry->getDisplayProperties()->getPadding();
    int iconOffset = (icon) ? pad.left + icon->getDimensions().x : 0;
    int maximumSliderArea = size.x - (iconOffset + where.x + pad.right);
    int filledAreaX = analogRangeToScreen(pItem, maximumSliderArea);
    int outsideAreaX = maximumSliderArea - filledAreaX;
    graphics->fillRect(where.x + iconOffset, where.y, filledAreaX, size.y, entry->getDisplayProperties()->getColor(ItemDisplayProperties::HIGHLIGHT1));
    graphics->fillRect(where.x + iconOffset + filledAreaX, where.y, outsideAreaX, size.y, entry->getDisplayProperties()->getColor(ItemDisplayProperties::HIGHLIGHT2));
    internalDrawText(entry, Coord(where.x + iconOffset, where.y), Coord(size.x - iconOffset, size.y));
}

void AdaFruitGfxMenuRenderer::drawIconItem(GridPositionRowCacheEntry* pEntry, Coord where, Coord size) {
    auto* props = pEntry->getDisplayProperties();
    auto* pItem = pEntry->getMenuItem();

    graphics->fillRect(where.x, where.y, size.x, size.y, props->getColor(ItemDisplayProperties::BACKGROUND));

    if(pItem->isActive()) {
        graphics->drawRect(where.x, where.y, size.x, size.y, props->getColor(ItemDisplayProperties::TEXT));
    }

    auto* pIcon = propertiesFactory.iconForMenuItem(pItem->getId());
    if(pIcon == nullptr) return;

    int xStart = where.x + ((size.x - pIcon->getDimensions().x) / 2);
    int yStart = where.y + pEntry->getDisplayProperties()->getPadding().top;

    bool sel = false;
    if(pItem->getMenuType() == MENUTYPE_BOOLEAN_VALUE) {
        auto* boolItem = reinterpret_cast<BooleanMenuItem*>(pItem);
        sel = boolItem->getBoolean();
    }
    if(pIcon->getIconType() == DrawableIcon::ICON_XBITMAP) {
        graphics->drawXBitmap(xStart, yStart, pIcon->getIcon(sel), pIcon->getDimensions().x, pIcon->getDimensions().y,
                              props->getColor(ItemDisplayProperties::HIGHLIGHT1));
    }
    else if(pIcon->getIconType() == DrawableIcon::DrawableIcon::ICON_MONO) {
        graphics->drawBitmap(xStart, yStart, pIcon->getIcon(sel), pIcon->getDimensions().x, pIcon->getDimensions().y,
                             props->getColor(ItemDisplayProperties::HIGHLIGHT1));
    }
    else if(pIcon->getIconType() == DrawableIcon::ICON_NATIVE) {
        graphics->drawRGBBitmap(xStart, yStart, (const uint16_t*)pIcon->getIcon(sel), pIcon->getDimensions().x, pIcon->getDimensions().y);
    }
}

void prepareAdaColorDefaultGfxConfig(AdaColorGfxMenuConfig* config) {
    prepareDefaultGfxConfig((ColorGfxMenuConfig<void*>*)config);
}

void prepareAdaMonoGfxConfigLoRes(AdaColorGfxMenuConfig* config) {
	makePadding(config->titlePadding, 2, 1, 1, 1);
	makePadding(config->itemPadding, 1, 1, 1, 1);
	makePadding(config->widgetPadding, 2, 2, 0, 2);

	config->bgTitleColor = BLACK;
	config->fgTitleColor = WHITE;
	config->titleFont = NULL;
	config->titleBottomMargin = 2;
	config->widgetColor = WHITE;
	config->titleFontMagnification = 1;

	config->bgItemColor = WHITE;
	config->fgItemColor = BLACK;
	config->bgSelectColor = BLACK;
	config->fgSelectColor = WHITE;
	config->itemFont = NULL;
	config->itemFontMagnification = 1;

    config->editIcon = loResEditingIcon;
    config->activeIcon = loResActiveIcon;
    config->editIconHeight = 6;
    config->editIconWidth = 8;
}

void prepareAdaMonoGfxConfigOled(AdaColorGfxMenuConfig* config) {
	makePadding(config->titlePadding, 2, 1, 1, 1);
	makePadding(config->itemPadding, 1, 1, 1, 1);
	makePadding(config->widgetPadding, 2, 2, 0, 2);

	config->bgTitleColor = WHITE;
	config->fgTitleColor = BLACK;
	config->titleFont = NULL;
	config->titleBottomMargin = 2;
	config->widgetColor = BLACK;
	config->titleFontMagnification = 1;

	config->bgItemColor = BLACK;
	config->fgItemColor = WHITE;
	config->bgSelectColor = WHITE;
	config->fgSelectColor = BLACK;
	config->itemFont = NULL;
	config->itemFontMagnification = 1;

    config->editIcon = loResEditingIcon;
    config->activeIcon = loResActiveIcon;
    config->editIconHeight = 6;
    config->editIconWidth = 8;
}

/** A couple of helper functions that we'll submit for inclusion once a bit more testing is done */

/**************************************************************************/
/*!
   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position,
   from image data that may be wider or taller than the desired width and height.
   Imagine a cookie dough rolled out, where you can cut a rectangle out of it.
   It uses the specified foreground (for set bits) and background (unset bits) colors.
   This is particularly useful for GFXCanvas1 operations, where you can allocate the
   largest canvas needed and then use it for all drawing operations.

    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   width of the portion you want to draw
    @param    h   Height of the portion you want to draw
    @param    totalWidth actual width of the bitmap
    @param    xStart X position of the image in the data
    @param    yStart Y position of the image in the data
    @param    color 16-bit 5-6-5 Color to draw pixels with
    @param    bg 16-bit 5-6-5 Color to draw background with
*/
/**************************************************************************/
void drawCookieCutBitmap(Adafruit_GFX* gfx, int16_t x, int16_t y, const uint8_t *bitmap, int16_t w,
                         int16_t h, int16_t totalWidth, int16_t xStart, int16_t yStart,
                         uint16_t fgColor, uint16_t bgColor) {

    // total width here is different to the width we are drawing, imagine rolling out a long
    // line of dough and cutting cookies from it. The cookie is the part of the image we want
    uint16_t byteWidth = (totalWidth + 7) / 8; // Bitmap scanline pad = whole byte
    uint16_t yEnd = h + yStart;
    uint16_t xEnd = w + xStart;
    uint8_t byte;

    gfx->startWrite();

    for (uint16_t j = yStart; j < yEnd; j++, y++) {
        byte = bitmap[size_t(((j * byteWidth) + xStart) / 8)];
        for (uint16_t i = xStart; i < xEnd; i++) {
            if (i & 7U)
                byte <<= 1U;
            else
                byte = bitmap[size_t((j * byteWidth) + i / 8)];
            gfx->writePixel(x + (i - xStart), y, (byte & 0x80U) ? fgColor : bgColor);
        }
    }

    gfx->endWrite();
}

/**************************************************************************/
/*!
   @brief      Draw a RAM-resident 2-bit image at the specified (x,y) position,
   from image data that may be wider or taller than the desired width and height.
   Imagine a cookie dough rolled out, where you can cut a rectangle out of it.
   It uses the specified foreground (for set bits) and background (unset bits) colors.
   This is particularly useful for GFXCanvas2 operations, where you can allocate the
   largest canvas needed and then use it for all drawing operations.

    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   width of the portion you want to draw
    @param    h   Height of the portion you want to draw
    @param    totalWidth actual width of the bitmap
    @param    xStart X position of the image in the data
    @param    yStart Y position of the image in the data
    @param    palette an array of 4 colours
*/
/**************************************************************************/
void drawCookieCutBitmap2(Adafruit_GFX* gfx, int16_t x, int16_t y, const uint8_t *bitmap, int16_t w,
                          int16_t h, int16_t totalWidth, int16_t xStart, int16_t yStart,
                          uint16_t* palette) {

    // total width here is different to the width we are drawing, imagine rolling out a long
    // line of dough and cutting cookies from it. The cookie is the part of the image we want
    uint16_t byteWidth = (totalWidth + 3) / 4; // Bitmap scanline pad = whole byte
    uint16_t yEnd = h + yStart;
    uint16_t xEnd = w + xStart;
    uint8_t byte;

    gfx->startWrite();

    for (uint16_t j = yStart; j < yEnd; j++, y++) {
        byte = bitmap[size_t(((j * byteWidth) + xStart) / 8)];
        for (uint16_t i = xStart; i < xEnd; i++) {
            if (i & 3U)
                byte <<= 2U;
            else
                byte = bitmap[size_t((j * byteWidth) + i / 8)];
            auto col = (byte & 0xC0U) >> 6U;
            gfx->writePixel(x + (i - xStart), y,  palette[col]);
        }
    }

    gfx->endWrite();
}
