import * as skin from '@common/skin';

import { Event, EventInterface } from 'microevent.ts';
import { GenericEventHandlingService, HasEvents } from '@common/service/GenericEventHandlingService';

import { Cloudpilot } from '@common/Cloudpilot';
import { DeviceId } from '@common/model/DeviceId';
import { DeviceOrientation } from '@common/model/DeviceOrientation';
import { EmbeddedCanvasDisplayService } from './service/EmbeddedCanvasDisplayService';
import { EmbeddedEmulationService } from './service/EmbeddedEmulationService';
import { EmulationStatistics } from '@common/model/EmulationStatistics';
import { Session } from './model/Session';
import { SessionMetadata } from '@common/model/SessionMetadata';
import { loadSkin } from '@common/skin/loader';

const DEFAULT_SESSION: Session = {
    hotsyncName: undefined,
    orientation: DeviceOrientation.protrait,
    runInBackground: false,
    speed: 1,
    deviceId: DeviceId.m515,
};

export interface EmulatorInterface {
    getError(): string | undefined;
    clearError(): this;

    loadRom(rom: Uint8Array, deviceId: DeviceId): this;

    loadSession(session: Uint8Array): this;

    setCanvas(canvas: HTMLCanvasElement): this;

    bindInput(pointerTarget: HasEvents, keyboardTarget?: HasEvents): this;
    releaseInput(): this;

    installDatabase(file: Uint8Array): this;
    installAndLaunchDatabase(file: Uint8Array): this;
    installFromZipfile(file: Uint8Array): this;

    launchByName(name: string): this;
    launchDatabase(database: Uint8Array): this;

    reset(): this;
    resetNoExtensions(): this;
    resetHard(): this;

    isRunning(): boolean;
    isPowerOff(): boolean;
    isUiInitialized(): boolean;

    mute(): this;
    unmute(): this;
    autounmute(): this;

    resume(): this;
    pause(): this;

    setSpeed(speed: number): this;
    getSpeed(): number;

    setVolume(volume: number): this;
    getVolume(): number;

    setOrientation(orientation: DeviceOrientation): this;
    getOrientation(): DeviceOrientation;

    setHotsyncName(hotsyncName: string | undefined): this;
    getHotsyncName(): string | undefined;

    setRunHidden(toggle: boolean): this;
    getRunHidden(): boolean;

    getStatistics(): EmulationStatistics;

    readonly powerOffChangeEvent: EventInterface<boolean>;
    readonly audioInitialized: EventInterface<void>;
}

export class Emulator implements EmulatorInterface {
    constructor(private cloudpilot: Cloudpilot) {
        this.emulationService = new EmbeddedEmulationService();
        this.canvasDisplayService = new EmbeddedCanvasDisplayService(loadSkin(Promise.resolve(skin)));
        this.eventHandlingService = new GenericEventHandlingService(this.emulationService, this.canvasDisplayService);

        this.emulationService.newFrameEvent.addHandler((canvas) =>
            this.canvasDisplayService.updateEmulationCanvas(canvas)
        );
    }

    getError(): string | undefined {
        return this.pendingError;
    }

    clearError(): this {
        this.pendingError = undefined;

        return this;
    }

    getStatistics(): EmulationStatistics {
        throw new Error('Method not implemented.');
    }

    loadRom(rom: Uint8Array, deviceId?: DeviceId): this {
        if (this.pendingError) return this;

        if (deviceId === undefined) {
            const rominfo = this.cloudpilot.getRomInfo(rom);
            if (!rominfo || rominfo.supportedDevices.length === 0) {
                return this.setError('unsupported device');
            }

            deviceId = rominfo.supportedDevices[0];
        }
        this.session = { ...DEFAULT_SESSION, deviceId };

        if (this.emulationService.initWithRom(this.cloudpilot, rom, deviceId, this.session)) {
            this.canvasDisplayService.initialize(undefined, deviceId, this.session.orientation);
            return this;
        }

        return this.setError('failed to initialize session');
    }

    loadSession(session: Uint8Array): this {
        if (this.pendingError) return this;

        const sessionImage = this.cloudpilot.deserializeSessionImage<SessionMetadata>(session);
        if (!sessionImage) return this.setError('bad session image');

        this.session = {
            hotsyncName: sessionImage.metadata?.hotsyncName,
            speed: sessionImage.metadata?.speed ?? 1,
            orientation: sessionImage.metadata?.deviceOrientation ?? DeviceOrientation.protrait,
            runInBackground: false,
            deviceId: sessionImage.deviceId,
        };

        if (this.emulationService.initWithSessionImage(this.cloudpilot, sessionImage, this.session)) {
            this.canvasDisplayService.initialize(undefined, sessionImage.deviceId, this.session.orientation);
            return this;
        }

        return this.setError('failed to initialize session');
    }

    setCanvas(canvas: HTMLCanvasElement): this {
        if (this.pendingError) return this;

        this.canvasDisplayService.initialize(canvas, this.session.deviceId, this.session.orientation);

        return this;
    }

    bindInput(pointerTarget: HasEvents, keyEventTarget?: HasEvents): this {
        if (this.pendingError) return this;

        this.eventHandlingService.bind(pointerTarget, keyEventTarget);

        return this;
    }

    releaseInput(): this {
        if (this.pendingError) return this;

        this.eventHandlingService.release();

        return this;
    }

    installDatabase(file: Uint8Array): this {
        if (this.pendingError) return this;

        return this.cloudpilot.installDb(file) >= 0 ? this : this.setError('failed to install database');
    }

    installAndLaunchDatabase(file: Uint8Array): this {
        if (this.pendingError) return this;

        this.installDatabase(file);
        if (this.getError() !== undefined) return this;

        this.launchDatabase(file);
        return this;
    }

    installFromZipfile(file: Uint8Array): this {
        throw new Error('Method not implemented.');
    }

    launchByName(name: string): this {
        if (this.pendingError) return this;

        return this.cloudpilot.launchAppByName(name) ? this : this.setError(`failed to launch ${name}`);
    }

    launchDatabase(database: Uint8Array): this {
        if (this.pendingError) return this;

        return this.cloudpilot.launchAppByDbHeader(database) ? this : this.setError('failed to launch database');
    }

    reset(): this {
        if (this.pendingError) return this;

        this.emulationService.reset();

        return this;
    }

    resetNoExtensions(): this {
        if (this.pendingError) return this;

        this.emulationService.resetNoExtensions();

        return this;
    }

    resetHard(): this {
        if (this.pendingError) return this;

        this.emulationService.resetHard();

        return this;
    }

    isRunning(): boolean {
        return this.emulationService.isRunning();
    }

    isPowerOff(): boolean {
        return this.emulationService.isPowerOff();
    }

    isUiInitialized(): boolean {
        return this.emulationService.isUiInitialized();
    }

    mute(): this {
        throw new Error('Method not implemented.');

        return this;
    }

    unmute(): this {
        throw new Error('Method not implemented.');

        return this;
    }

    autounmute(): this {
        throw new Error('Method not implemented.');

        return this;
    }

    resume(): this {
        if (this.pendingError) return this;

        this.emulationService.resume();

        return this;
    }

    pause(): this {
        if (this.pendingError) return this;

        this.emulationService.pause();

        return this;
    }

    setSpeed(speed: number): this {
        if (this.pendingError) return this;

        this.session.speed = speed;

        return this;
    }

    getSpeed(): number {
        return this.session.speed;
    }

    setVolume(volume: number): this {
        if (this.pendingError) return this;

        throw new Error('Method not implemented.');

        return this;
    }

    getVolume(): number {
        throw new Error('Method not implemented.');
    }

    setOrientation(orientation: DeviceOrientation): this {
        if (this.pendingError) return this;

        this.session.orientation = orientation;
        this.canvasDisplayService.updateOrientation(this.session.orientation);

        return this;
    }

    getOrientation(): DeviceOrientation {
        return this.session.orientation;
    }

    setHotsyncName(hotsyncName: string | undefined): this {
        if (this.pendingError) return this;

        this.session.hotsyncName = hotsyncName;

        return this;
    }

    getHotsyncName(): string | undefined {
        return this.session.hotsyncName;
    }

    setRunHidden(toggle: boolean): this {
        if (this.pendingError) return this;

        this.session.runInBackground = toggle;

        return this;
    }

    getRunHidden(): boolean {
        return this.session.runInBackground;
    }

    private setError(error: string): this {
        this.pendingError = error;
        return this;
    }

    readonly powerOffChangeEvent = new Event<boolean>();
    readonly audioInitialized = new Event<void>();

    private emulationService: EmbeddedEmulationService;
    private canvasDisplayService: EmbeddedCanvasDisplayService;
    private eventHandlingService: GenericEventHandlingService;

    private session: Session = { ...DEFAULT_SESSION };

    private pendingError: string | undefined = undefined;
}
